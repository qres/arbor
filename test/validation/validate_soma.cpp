#include <nlohmann/json.hpp>

#include <arbor/common_types.hpp>
#include <arbor/domain_decomposition.hpp>
#include <arbor/load_balance.hpp>
#include <arbor/mc_cell.hpp>
#include <arbor/recipe.hpp>
#include <arbor/simple_sampler.hpp>
#include <arbor/simulation.hpp>

#include "../common_cells.hpp"
#include "../simple_recipes.hpp"

#include "convergence_test.hpp"
#include "trace_analysis.hpp"
#include "util.hpp"
#include "validation_data.hpp"

#include "../gtest.h"

using namespace arb;

void validate_soma(backend_kind backend) {
    float sample_dt = g_trace_io.sample_dt();

    mc_cell c = make_cell_soma_only();

    cable1d_recipe rec{c};
    rec.add_probe(0, 0, cell_probe_address{{0, 0.5}, cell_probe_address::membrane_voltage});
    probe_label plabels[1] = {{"soma.mid", {0u, 0u}}};

    execution_context context;
    proc_allocation nd;
    nd.num_gpus = (backend==backend_kind::gpu);

    auto decomp = partition_load_balance(rec, nd, context);
    simulation sim(rec, decomp, context);

    nlohmann::json meta = {
        {"name", "membrane voltage"},
        {"model", "soma"},
        {"sim", "arbor"},
        {"units", "mV"},
        {"backend_kind", to_string(backend)}
    };

    convergence_test_runner<float> runner("dt", plabels, meta);
    runner.load_reference_data("numeric_soma.json");

    float t_end = 100.f;

    // use dt = 0.05, 0.02, 0.01, 0.005, 0.002,  ...
    double max_oo_dt = std::round(1.0/g_trace_io.min_dt());
    for (double base = 100; ; base *= 10) {
        for (double multiple: {5., 2., 1.}) {
            double oo_dt = base/multiple;
            if (oo_dt>max_oo_dt) goto end;

            sim.reset();
            float dt = float(1./oo_dt);
            runner.run(sim, dt, sample_dt, t_end, dt, {});
        }
    }
end:

    runner.report();
    runner.assert_all_convergence();
}

TEST(soma, numeric_ref) {
    execution_context ctx;
    validate_soma(backend_kind::multicore);
    if (local_allocation(ctx).num_gpus) {
        validate_soma(backend_kind::gpu);
    }
}

// DO NOT EDIT;
// Generated by ml/nn/runtime/test/specs/generate_test.sh
#include "../../TestGenerated.h"

namespace fully_connected_quant8_2 {
std::vector<MixedTypedExample> examples = {
// Generated fully_connected_quant8_2 test
#include "generated/examples/fully_connected_quant8_2.example.cpp"
};
// Generated model constructor
#include "generated/models/fully_connected_quant8_2.model.cpp"
} // namespace fully_connected_quant8_2
TEST_F(GeneratedTests, fully_connected_quant8_2) {
    execute(fully_connected_quant8_2::CreateModel,
            fully_connected_quant8_2::is_ignored,
            fully_connected_quant8_2::examples);
}
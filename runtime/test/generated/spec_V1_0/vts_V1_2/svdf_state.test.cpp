// Generated from svdf_state.mod.py
// DO NOT EDIT
// clang-format off
#include "GeneratedTests.h"

namespace generated_tests::svdf_state {

std::vector<::test_helper::MixedTypedExample>& get_examples();

} // namespace generated_tests::svdf_state

namespace android::hardware::neuralnetworks::V1_2::generated_tests::svdf_state {

Model createTestModel();
bool is_ignored(int);

TEST_F(NeuralnetworksHidlTest, svdf_state) {
  Execute(device,
          createTestModel,
          is_ignored,
          ::generated_tests::svdf_state::get_examples());
}

TEST_F(ValidationTest, svdf_state) {
  const Model model = createTestModel();
  const std::vector<Request> requests = createRequests(::generated_tests::svdf_state::get_examples());
  validateEverything(model, requests);
}

} // namespace android::hardware::neuralnetworks::V1_2::generated_tests::svdf_state

namespace generated_tests::svdf_state {

std::vector<::test_helper::MixedTypedExample>& get_examples_dynamic_output_shape();

} // namespace generated_tests::svdf_state

namespace android::hardware::neuralnetworks::V1_2::generated_tests::svdf_state {

Model createTestModel_dynamic_output_shape();
bool is_ignored_dynamic_output_shape(int);

TEST_F(DynamicOutputShapeTest, svdf_state_dynamic_output_shape) {
  Execute(device,
          createTestModel_dynamic_output_shape,
          is_ignored_dynamic_output_shape,
          ::generated_tests::svdf_state::get_examples_dynamic_output_shape(), true);
}

TEST_F(ValidationTest, svdf_state_dynamic_output_shape) {
  const Model model = createTestModel_dynamic_output_shape();
  const std::vector<Request> requests = createRequests(::generated_tests::svdf_state::get_examples_dynamic_output_shape());
  validateEverything(model, requests);
}

} // namespace android::hardware::neuralnetworks::V1_2::generated_tests::svdf_state

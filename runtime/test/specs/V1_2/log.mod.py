#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import math

input0 = Input("input0", "TENSOR_FLOAT32", "{1, 2, 3, 4, 5}")
output0 = Output("output0", "TENSOR_FLOAT32", "{1, 2, 3, 4, 5}")
model = Model().Operation("LOG", input0).To(output0)

input_data = [(i + 1) / 10 for i in range(120)]
output_data = [math.log(x) for x in input_data]

Example({
    input0: input_data,
    output0: output_data,
}).AddVariations("relaxed", "float16")

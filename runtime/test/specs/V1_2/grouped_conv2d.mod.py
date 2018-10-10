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

# TEST 1: GROUPED_CONV2D, pad = 0, stride = 1, numGroups = 2, act = relu
i1 = Input("op1", "TENSOR_FLOAT32", "{1, 3, 3, 2}") # input 0
w1 = Parameter("op2", "TENSOR_FLOAT32", "{2, 2, 2, 1}", [1, 2, 2, 1, 4, 3, 2, 1]) # weight
b1 = Parameter("op3", "TENSOR_FLOAT32", "{2}", [10, -35]) # bias
o1 = Output("op4", "TENSOR_FLOAT32", "{1, 2, 2, 2}") # output 0
Model().Operation("GROUPED_CONV_2D", i1, w1, b1, 0, 0, 0, 0, 1, 1, 2, 1).To(o1)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i1: ("TENSOR_QUANT8_ASYMM", 0.25, 100),
    w1: ("TENSOR_QUANT8_ASYMM", 0.25, 128),
    b1: ("TENSOR_INT32", 0.0625, 0),
    o1: ("TENSOR_QUANT8_ASYMM", 0.5, 80)
})

Example({
    i1: [1, 2, 3, 4, 5, 6,
         6, 5, 4, 3, 2, 1,
         2, 3, 3, 3, 3, 3],
    o1: [33, 0,
         33, 6,
         31, 3,
         27, 0]
}).AddVariations("relaxed", quant8).AddInput([w1, b1])


# TEST 2: GROUPED_CONV2D_LARGE, pad = same, stride = 1, numGroups = 2, act = none
i2 = Input("op1", "TENSOR_FLOAT32", "{1, 3, 2, 2}") # input 0
w2 = Parameter("op2", "TENSOR_FLOAT32", "{2, 2, 3, 1}", [100, 20, 1, 200, 10, 2, 200, 30, 1, 100, 20, 3]) # weight
b2 = Parameter("op3", "TENSOR_FLOAT32", "{2}", [500, -1000]) # bias
o2 = Output("op4", "TENSOR_FLOAT32", "{1, 3, 2, 2}") # output 0
Model("large").Operation("GROUPED_CONV_2D", i2, w2, b2, 1, 1, 1, 2, 0).To(o2)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i2: ("TENSOR_QUANT8_ASYMM", 0.25, 128),
    w2: ("TENSOR_QUANT8_ASYMM", 1.0, 0),
    b2: ("TENSOR_INT32", 0.25, 0),
    o2: ("TENSOR_QUANT8_ASYMM", 10.0, 100)
})

Example({
    i2: [1, 2, 3, 4,
         4, 3, 2, 1,
         2, 3, 3, 3],
    o2: [567, -873,
         1480, -160,
         608, -840,
         1370, -10,
         543, -907,
         760, -310]
}).AddVariations("relaxed", quant8).AddInput([w2, b2])


# TEST 3: GROUPED_CONV2D_CHANNEL, pad = same, stride = 1, numGroups = 3, act = none
i3 = Input("op1", "TENSOR_FLOAT32", "{1, 2, 2, 9}") # input 0
w3 = Parameter("op2", "TENSOR_FLOAT32", "{6, 1, 1, 3}", [1, 2, 3, 2, 1, 0, 2, 3, 3, 6, 6, 6, 9, 8, 5, 2, 1, 1]) # weight
b3 = Parameter("op3", "TENSOR_FLOAT32", "{6}", [10, -20, 30, -40, 50, -60]) # bias
o3 = Output("op4", "TENSOR_FLOAT32", "{1, 2, 2, 6}") # output 0
Model("channel").Operation("GROUPED_CONV_2D", i3, w3, b3, 1, 1, 1, 3, 0).To(o3)

# Additional data type
quant8 = DataTypeConverter().Identify({
    i3: ("TENSOR_QUANT8_ASYMM", 0.5, 0),
    w3: ("TENSOR_QUANT8_ASYMM", 0.25, 0),
    b3: ("TENSOR_INT32", 0.125, 0),
    o3: ("TENSOR_QUANT8_ASYMM", 2.0, 60)
})

Example({
    i3: [1, 2, 3, 4, 55, 4, 3, 2, 1,
         5, 4, 3, 2, 11, 2, 3, 4, 5,
         2, 3, 2, 3, 22, 3, 2, 3, 2,
         1, 0, 2, 1, 33, 1, 2, 0, 1],
    o3: [24, -16, 215, 338,  98, -51,
         32,  -6,  73,  50, 134, -45,
         24, -13, 111, 128, 102, -51,
         17, -18, 134, 170,  73, -55]
}).AddVariations("relaxed", quant8).AddInput([w3, b3])
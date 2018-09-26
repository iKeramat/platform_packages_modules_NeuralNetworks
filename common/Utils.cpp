/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Utils"

#include "NeuralNetworks.h"
#include "NeuralNetworksOEM.h"
#include "Utils.h"
#include "ValidateHal.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <sys/system_properties.h>
#include <unordered_map>

using ::android::hidl::allocator::V1_0::IAllocator;

namespace android {
namespace nn {

const char kVLogPropKey[] = "debug.nn.vlog";
int vLogMask = ~0;

// Split the space separated list of tags from verbose log setting and build the
// logging mask from it. note that '1' and 'all' are special cases to enable all
// verbose logging.
//
// NN API verbose logging setting comes from system property debug.nn.vlog.
// Example:
// setprop debug.nn.vlog 1 : enable all logging tags.
// setprop debug.nn.vlog "model compilation" : only enable logging for MODEL and
//                                             COMPILATION tags.
void initVLogMask() {
    vLogMask = 0;
    const std::string vLogSetting = android::base::GetProperty(kVLogPropKey, "");
    if (vLogSetting.empty()) {
        return;
    }

    std::unordered_map<std::string, int> vLogFlags = {
        {"1", -1},
        {"all", -1},
        {"model", MODEL},
        {"compilation", COMPILATION},
        {"execution", EXECUTION},
        {"cpuexe", CPUEXE},
        {"manager", MANAGER},
        {"driver", DRIVER}};

    std::vector<std::string> elements = android::base::Split(vLogSetting, " ,:");
    for (const auto& elem : elements) {
        const auto& flag = vLogFlags.find(elem);
        if (flag == vLogFlags.end()) {
            LOG(ERROR) << "Unknown trace flag: " << elem;
            continue;
        }

        if (flag->second == -1) {
            // -1 is used for the special values "1" and "all" that enable all
            // tracing.
            vLogMask = ~0;
            return;
        } else {
            vLogMask |= 1 << flag->second;
        }
    }
}

namespace {

template <typename EntryType, uint32_t entryCount, uint32_t entryCountOEM>
EntryType tableLookup(const EntryType (&table)[entryCount],
                      const EntryType (&tableOEM)[entryCountOEM],
                      uint32_t code) {
    if (code < entryCount) {
        return table[code];
    } else if (code >= kOEMCodeBase && (code - kOEMCodeBase) < entryCountOEM) {
        return tableOEM[code - kOEMCodeBase];
    } else {
        nnAssert(!"tableLookup: bad code");
        return EntryType();
    }
}

};  // anonymous namespace

#define COUNT(X) (sizeof(X) / sizeof(X[0]))

const char* kTypeNames[kNumberOfDataTypes] = {
        "FLOAT32", "INT32", "UINT32", "TENSOR_FLOAT32", "TENSOR_INT32", "TENSOR_QUANT8_ASYMM",
        "BOOL",
};

static_assert(COUNT(kTypeNames) == kNumberOfDataTypes, "kTypeNames is incorrect");

const char* kTypeNamesOEM[kNumberOfDataTypesOEM] = {
        "OEM",            "TENSOR_OEM_BYTE",
};

static_assert(COUNT(kTypeNamesOEM) == kNumberOfDataTypesOEM, "kTypeNamesOEM is incorrect");

const char* getOperandTypeName(OperandType type) {
    uint32_t n = static_cast<uint32_t>(type);
    return tableLookup(kTypeNames, kTypeNamesOEM, n);
}

// TODO Check if this useful
const char* kErrorNames[] = {
        "NO_ERROR", "OUT_OF_MEMORY", "INCOMPLETE", "NULL", "BAD_DATA",
};

const char* kOperationNames[kNumberOfOperationTypes] = {
        "ADD",
        "AVERAGE_POOL",
        "CONCATENATION",
        "CONV",
        "DEPTHWISE_CONV",
        "DEPTH_TO_SPACE",
        "DEQUANTIZE",
        "EMBEDDING_LOOKUP",
        "FLOOR",
        "FULLY_CONNECTED",
        "HASHTABLE_LOOKUP",
        "L2_NORMALIZATION",
        "L2_POOL",
        "LOCAL_RESPONSE_NORMALIZATION",
        "LOGISTIC",
        "LSH_PROJECTION",
        "LSTM",
        "MAX_POOL",
        "MUL",
        "RELU",
        "RELU1",
        "RELU6",
        "RESHAPE",
        "RESIZE_BILINEAR",
        "RNN",
        "SOFTMAX",
        "SPACE_TO_DEPTH",
        "SVDF",
        "TANH",
        "BATCH_TO_SPACE_ND",
        "DIV",
        "MEAN",
        "PAD",
        "SPACE_TO_BATCH_ND",
        "SQUEEZE",
        "STRIDED_SLICE",
        "SUB",
        "TRANSPOSE",
        "ARGMAX",
        "ARGMIN",
        "PAD_V2",
        "AXIS_ALIGNED_BBOX_TRANSFORM",
        "BIDIRECTIONAL_SEQUENCE_LSTM",
        "BIDIRECTIONAL_SEQUENCE_RNN",
        "BOX_WITH_NMS_LIMIT",
        "CAST",
        "CHANNEL_SHUFFLE",
        "DETECTION_OUTPUT",
        "EMBEDDING_LOOKUP_SPARSE",
        "EXP",
        "EXPAND_DIMS",
        "GATHER",
        "GENERATE_PROPOSALS",
        "GREATER",
        "GREATER_EQUAL",
        "GROUPED_CONV_2D",
        "HEATMAP_MAX_KEYPOINT",
        "LESS",
        "LESS_EQUAL",
        "LOG",
        "LOGICAL_AND",
        "LOGICAL_NOT",
        "LOGICAL_OR",
        "LOG_SOFTMAX",
        "MAXIMUM",
        "MINIMUM",
        "NEG",
        "POW",
        "PRELU",
        "PRIOR_BOX",
        "QUANTIZE",
        "QUANTIZED_16BIT_LSTM",
        "RANDOM_MULTINOMIAL",
        "REDUCE",
        "ROI_ALIGN",
        "RSQRT",
        "SELECT",
        "SIN",
        "SLICE",
        "SPARSE_TO_DENSE",
        "SPLIT",
        "SQRT",
        "TILE",
        "TOPK_V2",
        "TRANSPOSE_CONV_2D",
        "UNIDIRECTIONAL_SEQUENCE_LSTM",
        "UNIDIRECTIONAL_SEQUENCE_RNN",
        "ROTATED_BBOX_TRANSFORM",
};

static_assert(COUNT(kOperationNames) == kNumberOfOperationTypes, "kOperationNames is incorrect");

const char* kOperationNamesOEM[kNumberOfOperationTypesOEM] = {
        "OEM_OPERATION",
};

static_assert(COUNT(kOperationNamesOEM) == kNumberOfOperationTypesOEM,
              "kOperationNamesOEM is incorrect");

const char* getOperationName(OperationType type) {
    uint32_t n = static_cast<uint32_t>(type);
    return tableLookup(kOperationNames, kOperationNamesOEM, n);
}

const uint32_t kSizeOfDataType[]{
        4,  // ANEURALNETWORKS_FLOAT32
        4,  // ANEURALNETWORKS_INT32
        4,  // ANEURALNETWORKS_UINT32
        4,  // ANEURALNETWORKS_TENSOR_FLOAT32
        4,  // ANEURALNETWORKS_TENSOR_INT32
        1,  // ANEURALNETWORKS_TENSOR_SYMMETRICAL_QUANT8
        1,  // ANEURALNETWORKS_BOOL
};

static_assert(COUNT(kSizeOfDataType) == kNumberOfDataTypes, "kSizeOfDataType is incorrect");

const bool kScalarDataType[]{
        true,   // ANEURALNETWORKS_FLOAT32
        true,   // ANEURALNETWORKS_INT32
        true,   // ANEURALNETWORKS_UINT32
        false,  // ANEURALNETWORKS_TENSOR_FLOAT32
        false,  // ANEURALNETWORKS_TENSOR_INT32
        false,  // ANEURALNETWORKS_TENSOR_SYMMETRICAL_QUANT8
        true,   // ANEURALNETWORKS_BOOL
};

static_assert(COUNT(kScalarDataType) == kNumberOfDataTypes, "kScalarDataType is incorrect");

const uint32_t kSizeOfDataTypeOEM[]{
        0, // ANEURALNETWORKS_OEM
        1, // ANEURALNETWORKS_TENSOR_OEM_BYTE
};

static_assert(COUNT(kSizeOfDataTypeOEM) == kNumberOfDataTypesOEM,
              "kSizeOfDataTypeOEM is incorrect");

const bool kScalarDataTypeOEM[]{
        true,  // ANEURALNETWORKS_OEM
        false, // ANEURALNETWORKS_TENSOR_OEM_BYTE
};

static_assert(COUNT(kScalarDataTypeOEM) == kNumberOfDataTypesOEM,
              "kScalarDataTypeOEM is incorrect");

uint32_t sizeOfData(OperandType type, const std::vector<uint32_t>& dimensions) {
    int n = static_cast<int>(type);

    uint32_t size = tableLookup(kSizeOfDataType, kSizeOfDataTypeOEM, n);

    if (tableLookup(kScalarDataType, kScalarDataTypeOEM, n) == true) {
        return size;
    }

    for (auto d : dimensions) {
        size *= d;
    }
    return size;
}

hidl_memory allocateSharedMemory(int64_t size) {
    static const std::string type = "ashmem";
    static sp<IAllocator> allocator = IAllocator::getService(type);

    hidl_memory memory;

    // TODO: should we align memory size to nearest page? doesn't seem necessary...
    allocator->allocate(size, [&](bool success, const hidl_memory& mem) {
        if (!success) {
            LOG(ERROR) << "unable to allocate " << size << " bytes of " << type;
        } else {
            memory = mem;
        }
    });

    return memory;
}

uint32_t alignBytesNeeded(uint32_t index, size_t length) {
    uint32_t pattern;
    if (length < 2) {
        pattern = 0; // No alignment necessary
    } else if (length < 4) {
        pattern = 1; // Align on 2-byte boundary
    } else {
        pattern = 3; // Align on 4-byte boundary
    }
    uint32_t extra = (~(index - 1)) & pattern;
    return extra;
}

void logModelToInfo(const V1_0::Model& model) {
    LOG(INFO) << "V1_0::Model start";
    LOG(INFO) << "operands" << toString(model.operands);
    LOG(INFO) << "operations" << toString(model.operations);
    LOG(INFO) << "inputIndexes" << toString(model.inputIndexes);
    LOG(INFO) << "outputIndexes" << toString(model.outputIndexes);
    LOG(INFO) << "operandValues size" << model.operandValues.size();
    LOG(INFO) << "pools" << SHOW_IF_DEBUG(toString(model.pools));
}

void logModelToInfo(const V1_1::Model& model) {
    LOG(INFO) << "V1_1::Model start";
    LOG(INFO) << "operands" << toString(model.operands);
    LOG(INFO) << "operations" << toString(model.operations);
    LOG(INFO) << "inputIndexes" << toString(model.inputIndexes);
    LOG(INFO) << "outputIndexes" << toString(model.outputIndexes);
    LOG(INFO) << "operandValues size" << model.operandValues.size();
    LOG(INFO) << "pools" << SHOW_IF_DEBUG(toString(model.pools));
}

// Validates the type. The used dimensions can be underspecified.
int validateOperandType(const ANeuralNetworksOperandType& type, const char* tag,
                        bool allowPartial) {
    if (!allowPartial) {
        for (uint32_t i = 0; i < type.dimensionCount; i++) {
            if (type.dimensions[i] == 0) {
                LOG(ERROR) << tag << " OperandType invalid dimensions[" << i
                           << "] = " << type.dimensions[i];
                return ANEURALNETWORKS_BAD_DATA;
            }
        }
    }
    if (!validCode(kNumberOfDataTypes, kNumberOfDataTypesOEM, type.type)) {
        LOG(ERROR) << tag << " OperandType invalid type " << type.type;
        return ANEURALNETWORKS_BAD_DATA;
    }
    if (type.type == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        if (type.zeroPoint < 0 || type.zeroPoint > 255) {
            LOG(ERROR) << tag << " OperandType invalid zeroPoint " << type.zeroPoint;
            return ANEURALNETWORKS_BAD_DATA;
        }
        if (type.scale <= 0.f) {
            LOG(ERROR) << tag << " OperandType invalid scale " << type.scale;
            return ANEURALNETWORKS_BAD_DATA;
        }
    }
    if (type.type == ANEURALNETWORKS_FLOAT32 || type.type == ANEURALNETWORKS_INT32 ||
        type.type == ANEURALNETWORKS_UINT32 || type.type == ANEURALNETWORKS_BOOL ||
        type.type == ANEURALNETWORKS_OEM_SCALAR) {
        if (type.dimensionCount != 0 || type.dimensions != nullptr) {
            LOG(ERROR) << tag << " Invalid dimensions for scalar type";
            return ANEURALNETWORKS_BAD_DATA;
        }
    }

    return ANEURALNETWORKS_NO_ERROR;
}

int validateOperandList(uint32_t count, const uint32_t* list, uint32_t operandCount,
                        const char* tag) {
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] >= operandCount) {
            LOG(ERROR) << tag << " invalid operand index at " << i << " = " << list[i]
                       << ", operandCount " << operandCount;
            return ANEURALNETWORKS_BAD_DATA;
        }
    }
    return ANEURALNETWORKS_NO_ERROR;
}

int validateOperationOperandTypes(const std::vector<Operand>& operands,
                                  uint32_t inOperandCount, const uint32_t* inOperandIndexes,
                                  const std::vector<OperandType>& inExpectedTypes,
                                  uint32_t outOperandCount, const uint32_t* outOperandIndexes,
                                  const std::vector<OperandType>& outExpectedInTypes) {
    if (inOperandCount > static_cast<uint32_t>(inExpectedTypes.size()) ||
        outOperandCount > static_cast<uint32_t>(outExpectedInTypes.size())) {
        return ANEURALNETWORKS_BAD_DATA;
    }
    for (uint32_t i = 0; i < inOperandCount; i++) {
        if (operands[inOperandIndexes[i]].type != inExpectedTypes[i]) {
            LOG(ERROR) << "Invalid input tensor type "
                       << toString(operands[inOperandIndexes[i]].type)
                       << " for input " << i << ", expected " << toString(inExpectedTypes[i]);
            return ANEURALNETWORKS_BAD_DATA;
        }
    }
    for (uint32_t i = 0; i < outOperandCount; i++) {
        if (operands[outOperandIndexes[i]].type != outExpectedInTypes[i]) {
            LOG(ERROR) << "Invalid output tensor type "
                       << toString(operands[outOperandIndexes[i]].type)
                       << " for input " << i << ", expected " << toString(outExpectedInTypes[i]);
            return ANEURALNETWORKS_BAD_DATA;
        }
    }

    return ANEURALNETWORKS_NO_ERROR;
}

int validateOperation(ANeuralNetworksOperationType opType,
                      uint32_t inputCount, const uint32_t* inputIndexes,
                      uint32_t outputCount, const uint32_t* outputIndexes,
                      const std::vector<Operand>& operands) {
    int n = validateOperandList(inputCount, inputIndexes, static_cast<uint32_t>(operands.size()),
                                "ANeuralNetworksModel_addOperation inputs");
    if (n != ANEURALNETWORKS_NO_ERROR) {
        return n;
    }
    n = validateOperandList(outputCount, outputIndexes, static_cast<uint32_t>(operands.size()),
                            "ANeuralNetworksModel_addOperation outputs");
    if (n != ANEURALNETWORKS_NO_ERROR) {
        return n;
    }

    auto logInvalidInOutNumber = [opType, inputCount, outputCount](int expIn, int expOut) {
        LOG(ERROR) << "Invalid number of input operands ("
                   << inputCount << ", expected " << expIn << ") or output operands ("
                   << outputCount << ", expected " << expOut << ") for operation "
                   << kOperationNames[opType];
    };

    switch (opType) {
        case ANEURALNETWORKS_OEM_OPERATION: {
            return ANEURALNETWORKS_NO_ERROR;
        }
        case ANEURALNETWORKS_ADD: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_MUL: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_FLOOR: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_DEQUANTIZE: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_QUANTIZE: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {inputType};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_DEPTHWISE_CONV_2D: {
            if ((inputCount != 12 && inputCount != 11 && inputCount != 9 && inputCount != 8) ||
                outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 12, 11, 9 or 8) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount >= 11) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            if (inputCount == 12 || inputCount == 9) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_CONV_2D: {
            if ((inputCount != 11 && inputCount != 10 && inputCount != 8 && inputCount != 7) ||
                outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 11, 10, 8 or 7) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount >= 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            if (inputCount == 11 || inputCount == 8) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_AVERAGE_POOL_2D: {
            if ((inputCount != 11 && inputCount != 10 && inputCount != 8 && inputCount != 7) ||
                outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 11, 10, 8 or 7) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount >= 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            if (inputCount == 11 || inputCount == 8) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_L2_POOL_2D: {
            if ((inputCount != 11 && inputCount != 10 && inputCount != 8 && inputCount != 7) ||
                outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 11, 10, 8 or 7) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount >= 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            if (inputCount == 11 || inputCount == 8) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_MAX_POOL_2D: {
            if ((inputCount != 11 && inputCount != 10 && inputCount != 8 && inputCount != 7) ||
                outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 11, 10, 8 or 7) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount >= 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            if (inputCount == 11 || inputCount == 8) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RELU: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RELU1: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RELU6: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_TANH: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_LOGISTIC: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SOFTMAX: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_FULLY_CONNECTED: {
            if (inputCount != 4 || outputCount != 1) {
                logInvalidInOutNumber(4, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_CONCATENATION: {
            if (inputCount < 2 || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected at least 2) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes(inputCount, inputType);
            std::vector<OperandType> outExpectedTypes = {inputType};
            // The last one is the activation function.
            inExpectedTypes.back() = OperandType::INT32;
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_L2_NORMALIZATION: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_LOCAL_RESPONSE_NORMALIZATION: {
            if (inputCount != 5 || outputCount != 1) {
                logInvalidInOutNumber(5, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::FLOAT32,
                                   OperandType::FLOAT32,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RESHAPE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RESIZE_BILINEAR: {
            if ((inputCount != 4 && inputCount != 3) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 4 or 3) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            if (inputCount == 4) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_DEPTH_TO_SPACE: {
            if ((inputCount != 3 && inputCount != 2) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 3 or 2) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            if (inputCount == 3) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SPACE_TO_DEPTH: {
            if ((inputCount != 3 && inputCount != 2) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 3 or 2) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            if (inputCount == 3) {
                inExpectedTypes.push_back(OperandType::BOOL);
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_EMBEDDING_LOOKUP: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[1]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32,
                                                        inputType};
            std::vector<OperandType> outExpectedTypes = {inputType};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_HASHTABLE_LOOKUP: {
            if (inputCount != 3 || outputCount != 2) {
                logInvalidInOutNumber(3, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[2]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32,
                                                        OperandType::TENSOR_INT32,
                                                        inputType};
            std::vector<OperandType> outExpectedTypes = {inputType,
                                                         OperandType::TENSOR_QUANT8_ASYMM};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_LSH_PROJECTION: {
            if (inputCount != 4 || outputCount != 1) {
                logInvalidInOutNumber(4, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[1]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        inputType,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_INT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_LSTM: {
            if (inputCount != 23 || outputCount != 4) {
                logInvalidInOutNumber(23, 4);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32,
                                                        OperandType::FLOAT32,
                                                        OperandType::FLOAT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_RNN: {
            if (inputCount != 6 || outputCount != 2) {
                logInvalidInOutNumber(6, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SVDF: {
            if (inputCount != 7 || outputCount != 2) {
                logInvalidInOutNumber(7, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_BATCH_TO_SPACE_ND: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SPACE_TO_BATCH_ND: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_PAD: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_PAD_V2: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_INT32,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_INT32,
                                   OperandType::INT32};  // TODO(b/116699425): Make it UINT8.
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SQUEEZE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_TRANSPOSE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_STRIDED_SLICE: {
            if (inputCount != 7 || outputCount != 1) {
                logInvalidInOutNumber(7, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_DIV: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SUB: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_MEAN: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_ARGMAX:
        case ANEURALNETWORKS_ARGMIN: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {inputType, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_INT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_EXPAND_DIMS: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {inputType, OperandType::INT32};
                outExpectedTypes = {inputType};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_SPLIT: {
            if (inputCount != 3) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount << ", expected 3)"
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes = {inputType, OperandType::INT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes(outputCount, inputType);
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_ROI_ALIGN: {
            if (inputCount != 5 || outputCount != 1) {
                logInvalidInOutNumber(5, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                               OperandType::TENSOR_INT32, OperandType::FLOAT32, OperandType::INT32};
            outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_HEATMAP_MAX_KEYPOINT: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32};
            outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_GROUPED_CONV_2D: {
            if ((inputCount != 11 && inputCount != 8) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 11 or 8) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32, OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32,
                                   OperandType::INT32,          OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 11) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(), explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_CHANNEL_SHUFFLE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM, OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_TRANSPOSE_CONV_2D: {
            if ((inputCount != 10 && inputCount != 8) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands (" << inputCount
                           << ", expected 10 or 8) or output operands (" << outputCount
                           << ", expected 1) for operation " << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM, OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            std::vector<OperandType> argExpectedTypes;
            if (inputCount == 10) {
                argExpectedTypes = {OperandType::INT32, OperandType::INT32, OperandType::INT32,
                                    OperandType::INT32, OperandType::INT32, OperandType::INT32,
                                    OperandType::INT32};
            } else {
                argExpectedTypes = {OperandType::TENSOR_INT32, OperandType::INT32,
                                    OperandType::INT32, OperandType::INT32, OperandType::INT32};
            }
            inExpectedTypes.insert(inExpectedTypes.end(), argExpectedTypes.begin(),
                                   argExpectedTypes.end());
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_PRELU: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case ANEURALNETWORKS_TILE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32 ||
                inputType == OperandType::TENSOR_INT32 ||
                inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {inputType, OperandType::TENSOR_INT32};
                outExpectedTypes = {inputType};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands, inputCount, inputIndexes,
                                                 inExpectedTypes, outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        default:
            return ANEURALNETWORKS_BAD_DATA;
    }
}

ErrorStatus convertResultCodeToErrorStatus(int resultCode) {
    switch (resultCode) {
        case ANEURALNETWORKS_NO_ERROR:
            return ErrorStatus::NONE;

        case ANEURALNETWORKS_BAD_DATA:
        case ANEURALNETWORKS_UNEXPECTED_NULL:
            return ErrorStatus::INVALID_ARGUMENT;

        default:
            LOG(ERROR) << "Unknown result code " << resultCode
                       << " mapped to ErrorStatus::GENERAL_FAILURE";
            return ErrorStatus::GENERAL_FAILURE;
        case ANEURALNETWORKS_BAD_STATE:
        case ANEURALNETWORKS_INCOMPLETE:
        case ANEURALNETWORKS_OP_FAILED:
        case ANEURALNETWORKS_OUT_OF_MEMORY:
        case ANEURALNETWORKS_UNMAPPABLE:
            return ErrorStatus::GENERAL_FAILURE;
    }
}

int convertErrorStatusToResultCode(ErrorStatus status) {
    switch (status) {
        case ErrorStatus::NONE:
            return ANEURALNETWORKS_NO_ERROR;

        case ErrorStatus::INVALID_ARGUMENT:
            return ANEURALNETWORKS_BAD_DATA;

        default:
            LOG(ERROR) << "Unknown ErrorStatus " << toString(status)
                       << " mapped to ANEURALNETWORKS_OP_FAILED";
            return ANEURALNETWORKS_OP_FAILED;
        case ErrorStatus::DEVICE_UNAVAILABLE:
        case ErrorStatus::GENERAL_FAILURE:
        case ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
            return ANEURALNETWORKS_OP_FAILED;
    }
}

// Versioning

bool compliantWithV1_0(V1_0::OperationType) {
    return true;
}

bool compliantWithV1_0(V1_1::OperationType operation) {
    return validOperationType(static_cast<V1_0::OperationType>(operation));
}

bool compliantWithV1_1(V1_0::OperationType) {
    return true;
}

bool compliantWithV1_1(V1_1::OperationType) {
    return true;
}

bool compliantWithV1_0(V1_0::Capabilities) {
    return true;
}

bool compliantWithV1_0(const V1_1::Capabilities& capabilities) {
    return capabilities.relaxedFloat32toFloat16Performance.execTime ==
           capabilities.float32Performance.execTime
           &&
           capabilities.relaxedFloat32toFloat16Performance.powerUsage ==
           capabilities.float32Performance.powerUsage;
}

bool compliantWithV1_1(const V1_0::Capabilities&) {
    return true;
}

bool compliantWithV1_1(const V1_1::Capabilities&) {
    return true;
}

bool compliantWithV1_0(const V1_0::Operation&) {
    return true;
}

bool compliantWithV1_0(const V1_1::Operation& operation) {
    return compliantWithV1_0(operation.type);
}

bool compliantWithV1_1(const V1_0::Operation&) {
    return true;
}

bool compliantWithV1_1(const V1_1::Operation&) {
    return true;
}

static bool compliantWithV1_0(const hidl_vec<V1_1::Operation>& operations) {
    return std::all_of(operations.begin(), operations.end(),
                       [](const V1_1::Operation& operation) {
                           return compliantWithV1_0(operation);
                       });
}

bool compliantWithV1_0(const V1_0::Model&) {
    return true;
}

bool compliantWithV1_0(const V1_1::Model& model) {
    // In addition to new enumeration values being introduced in V1_1::Model, a
    // new flag was introduced to indicate whether or not float32 data can be
    // calculated using float16 units. This 'relaxComputationFloat32toFloat16'
    // flag is not relevant in whether a V1_1::Model is compliant with a
    // V1_0::Model because all 1.0 drivers require strict calculation by default
    // in the P NN runtime. Even if fp16 calculations are allowed, they can
    // still be computed by a strict fp32 driver.
    return compliantWithV1_0(model.operations);
}

bool compliantWithV1_1(const V1_0::Model&) {
    return true;
}

bool compliantWithV1_1(const V1_1::Model&) {
    return true;
}

V1_0::OperationType convertToV1_0(V1_0::OperationType type) {
    return type;
}

V1_0::OperationType convertToV1_0(V1_1::OperationType type) {
    if (!compliantWithV1_0(type)) {
        LOG(ERROR) << "Upcasting non-compliant type " << toString(type)
                   << " from V1_1::OperationType to V1_0::OperationType";
    }
    return static_cast<V1_0::OperationType>(type);
}

V1_1::OperationType convertToV1_1(V1_0::OperationType type) {
    return static_cast<V1_1::OperationType>(type);
}

V1_1::OperationType convertToV1_1(V1_1::OperationType type) {
    return type;
}

V1_0::Capabilities convertToV1_0(const V1_0::Capabilities& capabilities) {
    return capabilities;
}

V1_0::Capabilities convertToV1_0(const V1_1::Capabilities& capabilities) {
    if (!compliantWithV1_0(capabilities)) {
        LOG(ERROR) << "Upcasting non-compliant capabilities " << toString(capabilities)
                   << " from V1_1::Capabilities to V1_0::Capabilities";
    }
    return { .float32Performance = capabilities.float32Performance,
             .quantized8Performance = capabilities.quantized8Performance };
}

V1_1::Capabilities convertToV1_1(const V1_0::Capabilities& capabilities) {
    return { .float32Performance = capabilities.float32Performance,
             .quantized8Performance = capabilities.quantized8Performance,
             .relaxedFloat32toFloat16Performance = capabilities.float32Performance };
}

V1_1::Capabilities convertToV1_1(const V1_1::Capabilities& capabilities) {
    return capabilities;
}

V1_0::Operation convertToV1_0(const V1_0::Operation& operation) {
    return operation;
}

V1_0::Operation convertToV1_0(const V1_1::Operation& operation) {
    if (!compliantWithV1_0(operation)) {
        LOG(ERROR) << "Upcasting non-compliant operation " << toString(operation)
                   << " from V1_1::Operation to V1_0::Operation";
    }
    return {.type = convertToV1_0(operation.type),
            .inputs = operation.inputs,
            .outputs = operation.outputs};
}

V1_1::Operation convertToV1_1(const V1_0::Operation& operation) {
    return {.type = convertToV1_1(operation.type),
            .inputs = operation.inputs,
            .outputs = operation.outputs};
}

V1_1::Operation convertToV1_1(const V1_1::Operation& operation) {
    return operation;
}

static hidl_vec<V1_0::Operation> convertToV1_0(const hidl_vec<V1_1::Operation>& operations) {
    hidl_vec<V1_0::Operation> result(operations.size());
    std::transform(operations.begin(), operations.end(), result.begin(),
                   [](const V1_1::Operation& operation) { return convertToV1_0(operation); });
    return result;
}

static hidl_vec<V1_1::Operation> convertToV1_1(const hidl_vec<V1_0::Operation>& operations) {
    hidl_vec<V1_1::Operation> result(operations.size());
    std::transform(operations.begin(), operations.end(), result.begin(),
                   [](const V1_0::Operation& operation) { return convertToV1_1(operation); });
    return result;
}

V1_0::Model convertToV1_0(const V1_0::Model& model) {
    return model;
}

V1_0::Model convertToV1_0(const V1_1::Model& model) {
    if (!compliantWithV1_0(model)) {
        LOG(ERROR) << "Upcasting non-compliant model " << SHOW_IF_DEBUG(toString(model))
                   << " from V1_1::Model to V1_0::Model";
    }
    return {.operands = model.operands,
            .operations = convertToV1_0(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools};
}

V1_1::Model convertToV1_1(const V1_0::Model& model) {
    return {.operands = model.operands,
            .operations = convertToV1_1(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools,
            .relaxComputationFloat32toFloat16 = false};
}

V1_1::Model convertToV1_1(const V1_1::Model& model) {
    return model;
}

void logModelToInfo(const V1_2::Model& model) {
    LOG(INFO) << "V1_2::Model start";
    LOG(INFO) << "operands" << toString(model.operands);
    LOG(INFO) << "operations" << toString(model.operations);
    LOG(INFO) << "inputIndexes" << toString(model.inputIndexes);
    LOG(INFO) << "outputIndexes" << toString(model.outputIndexes);
    LOG(INFO) << "operandValues size" << model.operandValues.size();
    LOG(INFO) << "pools" << SHOW_IF_DEBUG(toString(model.pools));
}

bool compliantWithV1_0(V1_2::OperationType operation) {
    return validOperationType(static_cast<V1_0::OperationType>(operation));
}

bool compliantWithV1_1(V1_2::OperationType operation) {
    return validOperationType(static_cast<V1_1::OperationType>(operation));
}

bool compliantWithV1_0(const V1_2::Operation& operation) {
    return compliantWithV1_0(operation.type);
}

static bool compliantWithV1_0(const hidl_vec<V1_2::Operation>& operations) {
    return std::all_of(operations.begin(), operations.end(),
                       [](const V1_2::Operation& operation) {
                           return compliantWithV1_0(operation);
                       });
}

bool compliantWithV1_0(const V1_2::Model& model) {
    // See the comment in compliantWithV1_0(const V1_1::Model&).
    return compliantWithV1_0(model.operations);
}

bool compliantWithV1_1(const V1_2::Model&) {
    return true;
}

V1_0::OperationType convertToV1_0(V1_2::OperationType type) {
    if (!compliantWithV1_0(type)) {
        LOG(ERROR) << "Upcasting non-compliant type " << toString(type)
                   << " from V1_2::OperationType to V1_1::OperationType";
    }
    return static_cast<V1_0::OperationType>(type);
}

V1_1::OperationType convertToV1_1(V1_2::OperationType type) {
    if (!compliantWithV1_1(type)) {
        LOG(ERROR) << "Upcasting non-compliant type " << toString(type)
                   << " from V1_2::OperationType to V1_1::OperationType";
    }
    return static_cast<V1_1::OperationType>(type);
}

V1_2::OperationType convertToV1_2(V1_0::OperationType type) {
    return static_cast<V1_2::OperationType>(type);
}

V1_2::OperationType convertToV1_2(V1_1::OperationType type) {
    return static_cast<V1_2::OperationType>(type);
}

V1_0::Operation convertToV1_0(const V1_2::Operation& operation) {
    if (!compliantWithV1_0(operation)) {
        LOG(ERROR) << "Upcasting non-compliant operation " << toString(operation)
                   << " from V1_1::Operation to V1_0::Operation";
    }
    return {.type = convertToV1_0(operation.type),
            .inputs = operation.inputs,
            .outputs = operation.outputs};
}

V1_1::Operation convertToV1_1(const V1_2::Operation& operation) {
    return {.type = convertToV1_1(operation.type),
            .inputs = operation.inputs,
            .outputs = operation.outputs};
}

template <typename T>
V1_2::Operation convertToV1_2(const T& operation) {
    return {.type = convertToV1_2(operation.type),
            .inputs = operation.inputs,
            .outputs = operation.outputs};
}

static hidl_vec<V1_0::Operation> convertToV1_0(const hidl_vec<V1_2::Operation>& operations) {
    hidl_vec<V1_0::Operation> result(operations.size());
    std::transform(operations.begin(), operations.end(), result.begin(),
                   [](const V1_2::Operation& operation) { return convertToV1_0(operation); });
    return result;
}

static hidl_vec<V1_1::Operation> convertToV1_1(const hidl_vec<V1_2::Operation>& operations) {
    hidl_vec<V1_1::Operation> result(operations.size());
    std::transform(operations.begin(), operations.end(), result.begin(),
                   [](const V1_2::Operation& operation) { return convertToV1_1(operation); });
    return result;
}

template <typename T>
static hidl_vec<V1_2::Operation> convertToV1_2(const hidl_vec<T>& operations) {
    hidl_vec<V1_2::Operation> result(operations.size());
    std::transform(operations.begin(), operations.end(), result.begin(),
                   [](const T& operation) {
                       return convertToV1_2(operation);
                   });
    return result;
}

// We only need to convert from 1.0 and back since there wasn't any changes to
// Operand in 1.1
V1_2::OperandType convertToV1_2(const V1_0::OperandType& operandType) {
    return static_cast<V1_2::OperandType>(operandType);
}

bool compliantWithV1_0(const V1_2::OperandType& operandType) {
    return validOperandType(static_cast<V1_0::OperandType>(operandType));
}

V1_0::OperandType convertToV1_0(const V1_2::OperandType& operandType) {
    if (!compliantWithV1_0(operandType)) {
        LOG(ERROR) << "Upcasting non-compliant operand type " << toString(operandType)
                   << " from V1_2::Operand to V1_0::Operand";
    }
    return static_cast<V1_0::OperandType>(operandType);
}

// We only need to convert from 1.0 and back since there wasn't any changes to
// Operand in 1.1
V1_2::Operand convertToV1_2(const V1_0::Operand& operand) {
    return {.type = convertToV1_2(operand.type),
            .dimensions = operand.dimensions,
            .numberOfConsumers = operand.numberOfConsumers,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = operand.lifetime,
            .location = operand.location};
}

V1_2::Operand convertToV1_2(const V1_2::Operand& operand) {
    return operand;
}

V1_0::Operand convertToV1_0(const V1_2::Operand& operand) {
    return {.type = convertToV1_0(operand.type),
            .dimensions = operand.dimensions,
            .numberOfConsumers = operand.numberOfConsumers,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = operand.lifetime,
            .location = operand.location};
}

// We only need to convert from 1.0 and back since there wasn't any changes to
// Operand in 1.1
hidl_vec<V1_2::Operand> convertToV1_2(const hidl_vec<V1_0::Operand>& operands) {
    hidl_vec<V1_2::Operand> result(operands.size());
    std::transform(operands.begin(), operands.end(), result.begin(),
                   [](const V1_0::Operand& operand) { return convertToV1_2(operand); });
    return result;
}

hidl_vec<V1_2::Operand> convertToV1_2(const hidl_vec<V1_2::Operand>& operands) {
    return operands;
}

hidl_vec<V1_0::Operand> convertToV1_0(const hidl_vec<V1_2::Operand>& operands) {
    hidl_vec<V1_0::Operand> result(operands.size());
    std::transform(operands.begin(), operands.end(), result.begin(),
                   [](const V1_2::Operand& operand) { return convertToV1_0(operand); });
    return result;
}

V1_0::Model convertToV1_0(const V1_2::Model& model) {
    if (!compliantWithV1_0(model)) {
        LOG(ERROR) << "Upcasting non-compliant model " << SHOW_IF_DEBUG(toString(model))
                   << " from V1_1::Model to V1_0::Model";
    }
    return {.operands = convertToV1_0(model.operands),
            .operations = convertToV1_0(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools};
}

V1_1::Model convertToV1_1(const V1_2::Model& model) {
    return {.operands = convertToV1_0(model.operands),  // Operands in 1.1 and 1.0 are identical.
            .operations = convertToV1_1(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools,
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16};
}

V1_2::Model convertToV1_2(const V1_0::Model& model) {
    return {.operands = convertToV1_2(model.operands),
            .operations = convertToV1_2(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools,
            .relaxComputationFloat32toFloat16 = false};
}

V1_2::Model convertToV1_2(const V1_1::Model& model) {
    return {.operands = convertToV1_2(model.operands),
            .operations = convertToV1_2(model.operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
            .operandValues = model.operandValues,
            .pools = model.pools,
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16};
}

#ifdef NN_DEBUGGABLE
uint32_t getProp(const char* str, uint32_t defaultValue) {
    const std::string propStr = android::base::GetProperty(str, "");
    if (propStr.size() > 0) {
        return std::stoi(propStr);
    } else {
        return defaultValue;
    }
}
#endif  // NN_DEBUGGABLE

} // namespace nn
} // namespace android

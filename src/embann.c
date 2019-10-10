// SPDX-License-Identifier: GPL-2.0-only
/*
    embann.c - EMbedded Backpropogating Artificial Neural Network.
    Copyright Peter Frost 2019
*/

#include "embann.h"
#include "embann_log.h"


static int embann_initInputLayer(uint16_t numInputNeurons);
static int embann_initHiddenLayer(uint16_t numHiddenNeurons,
                                   uint8_t numHiddenLayers,
                                   uint16_t numInputNeurons);
static int embann_initOutputLayer(uint16_t numOutputNeurons,
                                   uint16_t numHiddenNeurons);

#define TAG "Embann Core"

int WEAK_FUNCTION main(int argc, char const *argv[])
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec ^ tv.tv_sec);  /* Seed the PRNG */
    
    EMBANN_ERROR_CHECK(embann_benchmark());
    EMBANN_ERROR_CHECK(embann_init(10U, 10U, 1U, 10U));
}


int embann_init(uint16_t numInputNeurons,
                 uint16_t numHiddenNeurons, 
                 uint8_t numHiddenLayers,
                 uint16_t numOutputNeurons)
{
    network = (network_t*) malloc(sizeof(network_t) + 
                                 (sizeof(hiddenLayer_t) * numHiddenLayers));
    CHECK_MALLOC(network);

    EMBANN_ERROR_CHECK(embann_initInputLayer(numInputNeurons));
    EMBANN_ERROR_CHECK(embann_initHiddenLayer(numHiddenNeurons,
                           numHiddenLayers,
                           numInputNeurons));
    EMBANN_ERROR_CHECK(embann_initOutputLayer(numOutputNeurons,
                           numHiddenNeurons));

    network->properties.numLayers = numHiddenLayers + 2U;
    network->properties.numHiddenLayers = numHiddenLayers;
    network->properties.networkResponse = 0U;

    return EOK;
}

static int embann_initInputLayer(uint16_t numInputNeurons)
{
    inputLayer_t* inputLayer = (inputLayer_t*) malloc(sizeof(inputLayer_t) + 
                                                (sizeof(uNeuron_t*) * numInputNeurons));
    CHECK_MALLOC(inputLayer);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    // MISRA C 2012 14.4 - deliberate cast from pointer to integer
    // cppcheck-suppress misra-c2012-11.4
    EMBANN_LOGI(TAG, "inputLayer: 0x%x, size: %ld", (uint32_t) inputLayer, sizeof(inputLayer_t) + 
                                                (sizeof(uNeuron_t*) * numInputNeurons));
    #pragma GCC diagnostic pop

    inputLayer->numNeurons = numInputNeurons;
    for (uint8_t i = 0; i < numInputNeurons; i++)
    {
        uNeuron_t* pNeuron = (uNeuron_t*) malloc(sizeof(uNeuron_t));
        inputLayer->neuron[i] = pNeuron;
        inputLayer->neuron[i]->activation = 0.0F;
    }
    network->inputLayer = inputLayer;

    EMBANN_LOGI(TAG, "done input");
    return EOK;
}

static int embann_initHiddenLayer(uint16_t numHiddenNeurons,
                                   uint8_t numHiddenLayers,
                                   uint16_t numInputNeurons)
{
    for (uint8_t i = 0; i < numHiddenLayers; i++)
    {
        hiddenLayer_t* hiddenLayer = (hiddenLayer_t*) malloc(sizeof(hiddenLayer_t) + 
                                                (sizeof(wNeuron_t*) * numHiddenNeurons));
        CHECK_MALLOC(hiddenLayer);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        // MISRA C 2012 14.4 - deliberate cast from pointer to integer
        // cppcheck-suppress misra-c2012-11.4
        EMBANN_LOGI(TAG, "hiddenlayer: 0x%x, size: %ld", (uint32_t) hiddenLayer, sizeof(hiddenLayer_t) + 
                                                (sizeof(wNeuron_t*) * numHiddenNeurons));
        #pragma GCC diagnostic pop

        for (uint16_t j = 0; j < numHiddenNeurons; j++)
        {    
            wNeuron_t* pNeuron = (wNeuron_t*) malloc(sizeof(wNeuron_t) + (sizeof(neuronParams_t*) * (uint16_t)((i == 0U) ? numInputNeurons : numHiddenNeurons)));
            hiddenLayer->neuron[j] = pNeuron;
            hiddenLayer->neuron[j]->activation = 0.0F;

            for (uint16_t k = 0; k < (uint16_t)((i == 0U) ? numInputNeurons : numHiddenNeurons); k++)
            {
                neuronParams_t* hiddenLayerParams = (neuronParams_t*) malloc(sizeof(neuronParams_t));
                CHECK_MALLOC(hiddenLayerParams);

                hiddenLayer->neuron[j]->params[k] = hiddenLayerParams;

                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
                // MISRA C 2012 14.4 - deliberate cast from pointer to integer
                // cppcheck-suppress misra-c2012-11.4
                EMBANN_LOGV(TAG, "params array: 0x%x, bias 0x%x, weight 0x%x", 
                                    (uint32_t) &hiddenLayer->neuron[j]->params[k],
                                    (uint32_t) &hiddenLayer->neuron[j]->params[k]->bias,
                                    (uint32_t) &hiddenLayer->neuron[j]->params[k]->weight);
                #pragma GCC diagnostic pop

                hiddenLayer->neuron[j]->params[k]->bias = RAND_WEIGHT();
                hiddenLayer->neuron[j]->params[k]->weight = RAND_WEIGHT();
            }
        }

        EMBANN_LOGI(TAG, "done hidden");

        network->hiddenLayer[i] = *hiddenLayer;

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        // MISRA C 2012 14.4 - deliberate cast from pointer to integer
        // cppcheck-suppress misra-c2012-11.4
        EMBANN_LOGI(TAG, "hiddenlayer[i]: 0x%x", (uint32_t) &network->hiddenLayer[i]);
        #pragma GCC diagnostic pop
    }
    return EOK;
}

static int embann_initOutputLayer(uint16_t numOutputNeurons,
                                   uint16_t numHiddenNeurons)
{
    outputLayer_t* outputLayer = (outputLayer_t*) malloc(sizeof(outputLayer_t) + 
                                                        (sizeof(wNeuron_t*) * numOutputNeurons));
    CHECK_MALLOC(outputLayer);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    // MISRA C 2012 14.4 - deliberate cast from pointer to integer
    // cppcheck-suppress misra-c2012-11.4
    EMBANN_LOGI(TAG, "outputLayer: 0x%x, size: %ld", (uint32_t) outputLayer, sizeof(outputLayer_t) + 
                                                (sizeof(wNeuron_t*) * numOutputNeurons));
    #pragma GCC diagnostic pop

    for (uint8_t i = 0; i < numOutputNeurons; i++)
    {
        wNeuron_t* pNeuron = (wNeuron_t*) malloc(sizeof(wNeuron_t) + (sizeof(neuronParams_t*) * numHiddenNeurons));
        outputLayer->neuron[i] = pNeuron;
        outputLayer->neuron[i]->activation = 0.0F;
        
        for (uint16_t j = 0; j < numHiddenNeurons; j++)
        {
            neuronParams_t* outputNeuronParams = (neuronParams_t*) malloc(sizeof(neuronParams_t));
            CHECK_MALLOC(outputNeuronParams);

            outputLayer->neuron[i]->params[j] = outputNeuronParams;
            
            outputLayer->neuron[i]->params[j]->bias = RAND_WEIGHT();
            outputLayer->neuron[i]->params[j]->weight = RAND_WEIGHT();
        }
    }
    outputLayer->numNeurons = numOutputNeurons;
    network->outputLayer = outputLayer;

    EMBANN_LOGI(TAG, "done output");
    return EOK;
}





int embann_inputLayer(uint16_t* networkResponse)
{
    embann_sumAndSquashInput(network->inputLayer->neuron, 
                             network->hiddenLayer[0].neuron,
                             network->inputLayer->numNeurons, 
                             network->hiddenLayer[0].numNeurons);

    EMBANN_LOGD(TAG, "Done Input -> 1st Hidden Layer");
    for (uint8_t i = 1; i < network->properties.numHiddenLayers; i++)
    {
        embann_sumAndSquash(network->hiddenLayer[i - 1U].neuron,
                            network->hiddenLayer[i].neuron,
                            network->hiddenLayer[i - 1U].numNeurons,
                            network->hiddenLayer[i].numNeurons);

        EMBANN_LOGD(TAG, "Done Hidden Layer %d -> Hidden Layer %d", i - 1U, i);
    }

    embann_sumAndSquash(
        network->hiddenLayer[network->properties.numHiddenLayers - 1U].neuron,
        network->outputLayer->neuron, 
        network->hiddenLayer[network->properties.numHiddenLayers - 1U].numNeurons,
        network->outputLayer->numNeurons);

    EMBANN_LOGD(TAG, "Done Hidden Layer %d -> Output Layer", network->properties.numHiddenLayers);

    EMBANN_ERROR_CHECK(embann_outputLayer(&network->properties.networkResponse));

    if (networkResponse == NULL)
    {
        // Deviation from MISRA C2012 15.5 for reasonably simple error return values
        // cppcheck-suppress misra-c2012-15.5
        return ENOENT;
    }

    *networkResponse = network->properties.networkResponse;
    return EOK;
}

int embann_sumAndSquash(wNeuron_t* Input[], wNeuron_t* Output[], uint16_t numInputs,
                           uint16_t numOutputs)
{
    for (uint16_t i = 0; i < numOutputs; i++)
    {
        Output[i]->activation = 0.0F; // Bias[i];
        for (uint16_t j = 0; j < numInputs; j++)
        {
            Output[i]->activation += Input[j]->activation * Output[i]->params[j]->weight;
        }
        Output[i]->activation = tanhf(Output[i]->activation * PI);

        EMBANN_LOGV(TAG, "i:%d This is the embann_SumAndSquash Output %.2f", i, Output[i]);
    }
    return EOK;
}

int embann_sumAndSquashInput(uNeuron_t* Input[], wNeuron_t* Output[], uint16_t numInputs,
                           uint16_t numOutputs)
{
    for (uint16_t i = 0; i < numOutputs; i++)
    {
        Output[i]->activation = 0; // Bias[i];
        for (uint16_t j = 0; j < numInputs; j++)
        {
            Output[i]->activation += Input[j]->activation * Output[i]->params[j]->weight;
        }
        Output[i]->activation = tanhf(Output[i]->activation * PI);

        EMBANN_LOGV(TAG, "i:%d This is the embann_SumAndSquash Output %.2f", i, Output[i]);
    }
    return EOK;
}



int embann_outputLayer(uint16_t* networkResponse)
{
    uint8_t mostLikelyOutput = 0;

    for (uint16_t i = 0; i < network->outputLayer->numNeurons; i++)
    {
        if (network->outputLayer->neuron[i] >
            network->outputLayer->neuron[mostLikelyOutput])
        {
            mostLikelyOutput = i;
        }
        EMBANN_LOGV(TAG, "i: %d neuron: %-3f likely: %d", i, network->outputLayer->neuron[i]->activation, mostLikelyOutput);
    }

    if (networkResponse == NULL)
    {
        // Deviation from MISRA C2012 15.5 for reasonably simple error return values
        // cppcheck-suppress misra-c2012-15.5
        return ENOENT;
    }
    
    *networkResponse = mostLikelyOutput;
    return EOK;
}

int embann_benchmark(void)
{    
    uint16_t numElements = (random() % 2) + 300;
    
    int32_t MAX_ALIGNMENT testInt[numElements];
    float MAX_ALIGNMENT testFloat[numElements];
    double MAX_ALIGNMENT testDouble[numElements];
    int32_t MAX_ALIGNMENT testIntWeight[numElements];
    float MAX_ALIGNMENT testFloatWeight[numElements];
    double MAX_ALIGNMENT testDoubleWeight[numElements];
    int32_t MAX_ALIGNMENT testIntBias[numElements];
    float MAX_ALIGNMENT testFloatBias[numElements];
    double MAX_ALIGNMENT testDoubleBias[numElements];
    struct timeval timeBefore;
    struct timeval timeAfter;
    struct timeval timeDiff;

    for (uint16_t i = 0; i < NUM_ARRAY_ELEMENTS(testInt); i++)
    {
        testInt[i] = INT32_MAX;
        testIntBias[i] = (random() % 21) - 10;
        testIntWeight[i] = (random() % 1000) + 1;  
    }
    
    for (uint16_t i = 0; i < NUM_ARRAY_ELEMENTS(testFloat); i++)
    {
        testFloat[i] = FLT_MAX;
        testFloatBias[i] = (float)(random() % 21) - 10;
        testFloatWeight[i] = RAND_WEIGHT();
    }

    for (uint16_t i = 0; i < NUM_ARRAY_ELEMENTS(testDouble); i++)
    {
        testDouble[i] = DBL_MAX;
        testDoubleBias[i] = (double)(random() % 21) - 10;
        testDoubleWeight[i] = RAND_WEIGHT();
    }

    gettimeofday(&timeBefore, NULL);
    //#pragma omp parallel for
    for (int32_t i = 0; i < 100000; i++)
    {
        for (uint16_t j = 0; j < NUM_ARRAY_ELEMENTS(testInt); j++)
        {
            testInt[j] /= testIntWeight[j];
            testInt[j] += testIntBias[j];
        }
    }
    gettimeofday(&timeAfter, NULL);
    timersub(&timeAfter, &timeBefore, &timeDiff);
    EMBANN_LOGI(TAG, "Integer time was %ld microseconds, result %d", timeDiff.tv_usec, testInt[0]);

    gettimeofday(&timeBefore, NULL);
    //#pragma omp parallel for
    for (int32_t i = 0; i < 100000; i++)
    {
        for (uint16_t j = 0; j < NUM_ARRAY_ELEMENTS(testFloat); j++)
        {
            testFloat[j] *= testFloatWeight[j];
            testFloat[j] += testFloatBias[j];
        }
    }
    gettimeofday(&timeAfter, NULL);
    timersub(&timeAfter, &timeBefore, &timeDiff);
    EMBANN_LOGI(TAG, "Float time was %ld microseconds, result %.2f", timeDiff.tv_usec, testFloat[0]);

    gettimeofday(&timeBefore, NULL);
    //#pragma omp parallel for
    for (int32_t i = 0; i < 100000; i++)
    {
        for (uint16_t j = 0; j < NUM_ARRAY_ELEMENTS(testDouble); j++)
        {
            testDouble[j] *= testDoubleWeight[j];
            testDouble[j] += testDoubleBias[j];
        }
    }
    gettimeofday(&timeAfter, NULL);
    timersub(&timeAfter, &timeBefore, &timeDiff);
    EMBANN_LOGI(TAG, "Double time was %ld microseconds, result %.2f", timeDiff.tv_usec, testDouble[0]);

    return EOK;
}

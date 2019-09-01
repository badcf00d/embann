/*
  Embann.cpp - EMbedded Backpropogating Artificial Neural Network.
  Created by Peter Frost, 27th August 2019
*/

#include "embann.h"


static network_t* network;
static void embann_calculateInputNeurons(void);
static void embann_initInputLayer(uint16_t numInputNeurons);
static void embann_initHiddenLayer(uint16_t numHiddenNeurons,
                                   uint8_t numHiddenLayers,
                                   uint16_t numInputNeurons);
static void embann_initOutputLayer(uint16_t numOutputNeurons,
                                   uint16_t numHiddenNeurons);
#ifndef ARDUINO
static uint32_t millis(void);
#endif

/* Random float between -1 and 1 */
#define RAND_WEIGHT() ((float)rand() / (RAND_MAX / 2)) - 1
/* Throw an error if malloc failed */
#define CHECK_MALLOC(a) if (!a) {abort();}

void embann_init(uint16_t numInputNeurons,
                 uint16_t numHiddenNeurons, 
                 uint8_t numHiddenLayers,
                 uint16_t numOutputNeurons)
{
    network = (network_t*) malloc(sizeof(network_t) + 
                                 (sizeof(hiddenLayer_t) * numHiddenLayers));
    CHECK_MALLOC(network);

    embann_initInputLayer(numInputNeurons);
    embann_initHiddenLayer(numHiddenNeurons,
                           numHiddenLayers,
                           numInputNeurons);
    embann_initOutputLayer(numOutputNeurons,
                           numHiddenNeurons);

    network->properties.numLayers = numHiddenLayers + 2;
    network->properties.numHiddenLayers = numHiddenLayers;
    network->properties.networkResponse = 0;

    network->inputLayer.numNeurons = numInputNeurons;
    network->inputLayer.groupThresholds = (uint16_t*) malloc(numInputNeurons * sizeof(uint16_t));
    network->inputLayer.groupTotal = (uint16_t*) malloc(numInputNeurons * sizeof(uint16_t));

    embann_calculateInputNeurons();

    network->outputLayer.numNeurons = numOutputNeurons;
}

static void embann_initInputLayer(uint16_t numInputNeurons)
{
    inputLayer_t* inputLayer = (inputLayer_t*) malloc(sizeof(inputLayer_t) + 
                                                    (sizeof(uNeuron_t) * numInputNeurons));
    CHECK_MALLOC(inputLayer);

    for (uint8_t i = 0; i < numInputNeurons; i++)
    {
        inputLayer->neuron[i]->activation = 0.0F;
    }
    network->inputLayer = *inputLayer;
}

static void embann_initHiddenLayer(uint16_t numHiddenNeurons,
                                   uint8_t numHiddenLayers,
                                   uint16_t numInputNeurons)
{
    for (uint8_t i = 0; i < numHiddenLayers; i++)
    {
        hiddenLayer_t* hiddenLayer = (hiddenLayer_t*) malloc(sizeof(hiddenLayer_t) + 
                                                (sizeof(wNeuron_t) * numHiddenNeurons));
        CHECK_MALLOC(hiddenLayer);
        neuronParams_t* hiddenLayerParams;

        if (i == 0)
        {
            hiddenLayerParams = (neuronParams_t*) malloc(sizeof(neuronParams_t) * numInputNeurons);
        }
        else
        {
            hiddenLayerParams = (neuronParams_t*) malloc(sizeof(neuronParams_t) * numHiddenNeurons);
        }
        CHECK_MALLOC(hiddenLayerParams);

        for (uint16_t j = 0; j < numHiddenNeurons; j++)
        {
            hiddenLayer->neuron[j]->activation = 0.0F;
            
            for (uint16_t k = 0; k < numInputNeurons; k++)
            {
                hiddenLayer->neuron[j]->params[k]->bias = RAND_WEIGHT();
                hiddenLayer->neuron[j]->params[k]->weight = RAND_WEIGHT();
            }
        }

        network->hiddenLayer[i] = *hiddenLayer;
    }
}

static void embann_initOutputLayer(uint16_t numOutputNeurons,
                                   uint16_t numHiddenNeurons)
{
    outputLayer_t* outputLayer = (outputLayer_t*) malloc(sizeof(outputLayer_t) + 
                                                        (sizeof(wNeuron_t) * numOutputNeurons));
    CHECK_MALLOC(outputLayer);
    neuronParams_t* outputNeuronParams = (neuronParams_t*) malloc(sizeof(neuronParams_t) * numHiddenNeurons);
    CHECK_MALLOC(outputNeuronParams);

    for (uint8_t i = 0; i < numOutputNeurons; i++)
    {
        for (uint16_t j = 0; j < numHiddenNeurons; j++)
        {
            outputLayer->neuron[i]->params[j]->bias = RAND_WEIGHT();
            outputLayer->neuron[i]->params[j]->weight = RAND_WEIGHT();
        }
    }
    network->outputLayer = *outputLayer;
}

void embann_newInputRaw(uint16_t rawInputArray[], uint16_t numInputs)
{
    network->inputLayer.rawInputs = rawInputArray;
    network->inputLayer.numRawInputs = numInputs;
    embann_calculateInputNeurons();
}

void embann_newInputStruct(networkSampleBuffer_t sampleBuffer, uint16_t numInputs)
{
    network->inputLayer.rawInputs = sampleBuffer.samples;
    network->inputLayer.numRawInputs = numInputs;
    embann_calculateInputNeurons();
}

void embann_calculateInputNeurons()
{
    uint8_t largestGroup = 0;

    for (uint16_t i = 0; i < network->inputLayer.numNeurons; i++)
    {
        network->inputLayer.groupThresholds[i] =
            ((network->inputLayer.maxInput + 1) / network->inputLayer.numNeurons) *
            (i + 1);
        network->inputLayer.groupTotal[i] = 0;
        // printf(network->inputLayer.groupThresholds[i]);
    }

    for (uint16_t i = 0; i < network->inputLayer.numRawInputs; i++)
    {
        for (uint16_t j = 0; j < network->inputLayer.numNeurons; j++)
        {
            if (network->inputLayer.rawInputs[i] <=
                network->inputLayer.groupThresholds[j])
            {
                // printf("%d + 1 in group %d, ",
                // network->inputLayer.groupTotal[j],
                //              j);
                network->inputLayer.groupTotal[j] += 1;
                break;
            }
        }
    }

    for (uint16_t i = 0; i < network->inputLayer.numNeurons; i++)
    {
        if (network->inputLayer.groupTotal[i] >
            network->inputLayer.groupTotal[largestGroup])
        {
            largestGroup = i;
        }
    }

    for (uint16_t i = 0; i < network->inputLayer.numNeurons; i++)
    {
        // printf("group total i: %u group total largest: %u ",
        //              network->inputLayer.groupTotal[i],
        //              network->inputLayer.groupTotal[largestGroup]);
        network->inputLayer.neuron[i] = network->inputLayer.groupTotal[i] /
                                        network->inputLayer.groupTotal[largestGroup];
        // printf("input neuron %d = %.3f, ", i,
        // network->inputLayer.neuron[i]);
    }
}

uint8_t embann_inputLayer()
{
    embann_sumAndSquash(network->inputLayer.neuron, 
                        network->hiddenLayer[0].neuron,
                        network->inputLayer.numNeurons, 
                        network->hiddenLayer[0].numNeurons);
    // printf("Done Input -> 1st Hidden Layer");
    for (uint8_t i = 1; i < network->properties.numHiddenLayers; i++)
    {
        embann_sumAndSquash(network->hiddenLayer[i - 1].neuron,
                            network->hiddenLayer[i].neuron,
                            network->hiddenLayer[i - 1].numNeurons,
                            network->hiddenLayer[i].numNeurons);
        // printf("Done Hidden Layer %d -> Hidden Layer %d\n", i - 1, i);
    }

    embann_sumAndSquash(
        network->hiddenLayer[network->properties.numHiddenLayers - 1].neuron,
        network->outputLayer.neuron, 
        network->hiddenLayer[network->properties.numHiddenLayers - 1].numNeurons,
        network->outputLayer.numNeurons);

    /*printf("Done Hidden Layer %d -> Output Layer\n",
                network->properties.numHiddenLayers);*/

    network->properties.networkResponse = embann_outputLayer();
    return network->properties.networkResponse;
}

void embann_sumAndSquash(wNeuron_t* Input[], wNeuron_t* Output[], uint16_t numInputs,
                           uint16_t numOutputs)
{
    for (uint16_t i = 0; i < numOutputs; i++)
    {
        Output[i]->activation = 0; // Bias[i];
        for (uint16_t j = 0; j < numInputs; j++)
        {
            Output[i]->activation += Input[j]->activation * Output[i]->params[j]->weight;
        }
        Output[i]->activation = tanh(Output[i]->activation * PI);

        // tanh is a quicker alternative to sigmoid
        // printf("i:%d This is the embann_SumAndSquash Output %.2f\n", i,
        // Output[i]);
    }
}

uint8_t embann_outputLayer()
{
    uint8_t mostLikelyOutput = 0;

    for (uint16_t i = 0; i < network->outputLayer.numNeurons; i++)
    {
        if (network->outputLayer.neuron[i] >
            network->outputLayer.neuron[mostLikelyOutput])
        {
            mostLikelyOutput = i;
        }
        // printf("i: %d neuron: %-3f likely: %d\n", i,
        // network->outputLayer.neurons[i], mostLikelyOutput);
    }
    return mostLikelyOutput;
}

void embann_printNetwork()
{
    printf("\nInput: [");
    for (uint16_t i = 0; i < (network->inputLayer.numRawInputs - 1); i++)
    {
        printf("%d, ", network->inputLayer.rawInputs[i]);
    }
    printf("%d]",network->inputLayer.rawInputs[network->inputLayer.numRawInputs - 1]);

    printf("\nInput Layer | Hidden Layer ");
    if (network->properties.numHiddenLayers > 1)
    {
        printf("1 ");
        for (uint8_t i = 2; i <= network->properties.numHiddenLayers; i++)
        {
            printf("| Hidden Layer %d ", i);
        }
    }
    printf("| Output Layer");

    bool nothingLeft = false;
    uint16_t i = 0;
    while (nothingLeft == false)
    { /* TODO, Make this compatible with multiple hidden layers */
        if ((i >= network->inputLayer.numNeurons) &&
            (i >= network->hiddenLayer[0].numNeurons) &&
            (i >= network->outputLayer.numNeurons))
        {
            nothingLeft = true;
        }
        else
        {
            if (i < network->inputLayer.numNeurons)
            {
                printf("%-12.3f| ", network->inputLayer.neuron[i]);
            }
            else
            {
                printf("            | ");
            }

            if (i < network->hiddenLayer[0].numNeurons)
            {
                if (network->properties.numHiddenLayers == 1)
                {
                    printf("%-13.3f| ", network->hiddenLayer[0].neuron[i]);
                }
                else
                {
                    for (uint8_t j = 0; j < network->properties.numHiddenLayers; j++)
                    {
                        printf("%-15.3f| ", network->hiddenLayer[j].neuron[i]);
                    }
                }
            }
            else
            {
                printf("             | ");
                if (network->properties.numHiddenLayers > 1)
                {
                    printf("              | ");
                }
            }

            if (i < network->outputLayer.numNeurons)
            {
                printf("%.3f", network->outputLayer.neuron[i]);
            }
        }
        printf("\n");
        i++;
    }

    printf("I think this is output %d ", network->properties.networkResponse);
}

void embann_trainDriverInTime(float learningRate, bool verbose,
                          uint8_t numTrainingSets, uint8_t inputPin,
                          uint16_t bufferSize, long numSeconds)
{
    char serialInput[10];
    uint16_t trainingData[network->outputLayer.numNeurons][numTrainingSets]
                         [bufferSize],
        randomOutput, randomTrainingSet;

    for (uint8_t i = 0; i < network->outputLayer.numNeurons; i++)
    {
        printf("\nAttach the sensor to material %u: ", i);
        scanf("%s", serialInput);
        while (serialInput[0] == 0)
        {
            scanf("%s", serialInput);
        }
        printf("\nReading...");
        for (uint8_t j = 0; j < numTrainingSets; j++)
        {
            printf("%u...", j + 1);
            for (uint16_t k = 0; k < bufferSize; k++)
            {
                trainingData[i][j][k] = analogRead(inputPin);
                printf("%u, ", trainingData[i][j][k]);
            }
            printf("\n");
            delay(50);
        }
    }

    if (verbose == true)
    {
        printf("\nOutput Errors: \n");
    }

    unsigned long startTime = millis();
    numSeconds *= 1000;

    while ((millis() - startTime) < numSeconds)
    {
        randomOutput = rand() % network->outputLayer.numNeurons;
        randomTrainingSet = rand() % numTrainingSets;
        embann_newInputRaw(trainingData[randomOutput][randomTrainingSet], bufferSize);
        embann_inputLayer();

        if (verbose == true)
        {
            embann_errorReporting(randomOutput);
            printf("%u | %u ", randomOutput, randomTrainingSet);
        }

        embann_train(randomOutput, learningRate);
    }
}

void embann_trainDriverInError(float learningRate, bool verbose,
                          uint8_t numTrainingSets, uint8_t inputPin,
                          uint16_t bufferSize, float desiredCost)
{
    char serialInput[10];
    uint16_t trainingData[network->outputLayer.numNeurons][numTrainingSets]
                         [bufferSize],
        randomOutput, randomTrainingSet;
    float currentCost[network->outputLayer.numNeurons];
    bool converged = false;

    for (uint8_t i = 0; i < network->outputLayer.numNeurons; i++)
    {
        printf("\nAttach the sensor to material %u: ", i);
        scanf("%s", serialInput);
        while (serialInput[0] == 0)
        {
            scanf("%s", serialInput);
        }
        printf("Reading...");
        for (uint8_t j = 0; j < numTrainingSets; j++)
        {
            printf("%u...", j + 1);
            for (uint8_t k = 0; k < bufferSize; k++)
            {
                trainingData[i][j][k] = analogRead(inputPin);
                printf("%u, ", trainingData[i][j][k]);
            }
            printf("\n");
            delay(50);
        }
    }

    if (verbose == true)
    {
        printf("\nOutput Errors: \n");
    }

    while (!converged)
    {
        randomOutput = rand() % network->outputLayer.numNeurons;
        randomTrainingSet = rand() % numTrainingSets;
        currentCost[randomOutput] = 0.0;
        embann_newInputRaw(trainingData[randomOutput][randomTrainingSet], bufferSize);
        embann_inputLayer();

        if (verbose == true)
        {
            embann_errorReporting(randomOutput);
            printf("%u | %u ", randomOutput, randomTrainingSet);
        }

        embann_train(randomOutput, learningRate);
        for (uint8_t i = 0; i < network->outputLayer.numNeurons; i++)
        {
            if (i == randomOutput)
            {
                currentCost[randomOutput] += pow(1 - network->outputLayer.neuron[i]->activation, 2);
            }
            else
            {
                currentCost[randomOutput] += pow(network->outputLayer.neuron[i]->activation, 2);
            }
        }
        currentCost[randomOutput] /= network->outputLayer.numNeurons;

        for (uint8_t i = 0; i < network->outputLayer.numNeurons; i++)
        {
            printf("%f", currentCost[i]);
            printf(", ");
            if (currentCost[i] > desiredCost)
            {
                break;
            }
            if (i == (network->outputLayer.numNeurons - 1))
            {
                converged = true;
            }
        }
        printf("%f", desiredCost);
    }
}

void embann_train(uint8_t correctOutput, float learningRate)
{
    float dOutputErrorToOutputSum[network->outputLayer.numNeurons];
    float dTotalErrorToHiddenNeuron = 0.0;
    /* TODO, add support for multiple hidden layers */
    float outputNeuronWeightChange[network->outputLayer.numNeurons]
                                  [network->hiddenLayer[0].numNeurons];

    for (uint16_t i = 0; i < network->outputLayer.numNeurons; i++)
    {
        if (i == correctOutput)
        {
            dOutputErrorToOutputSum[i] =
                (1 - network->outputLayer.neuron[i]->activation) *
                embann_tanhDerivative(network->outputLayer.neuron[i]->activation);
        }
        else
        {
            dOutputErrorToOutputSum[i] =
                -network->outputLayer.neuron[i]->activation *
                embann_tanhDerivative(network->outputLayer.neuron[i]->activation);
        }
        // printf("\ndOutputErrorToOutputSum[%d]: %.3f", i,
        // dOutputErrorToOutputSum[i]);
        for (uint16_t j = 0; j < network->hiddenLayer[0].numNeurons; j++)
        {
            outputNeuronWeightChange[i][j] =
                dOutputErrorToOutputSum[i] *
                network->hiddenLayer[network->properties.numHiddenLayers - 1].neuron[j]->activation *
                learningRate;
            // printf("\n  outputNeuronWeightChange[%d][%d]: %.3f", i, j,
            //              outputNeuronWeightChange[i][j]);
        }
    }

    for (uint16_t i = 0; i < network->hiddenLayer[0].numNeurons; i++)
    {
        dTotalErrorToHiddenNeuron = 0.0;
        for (uint16_t j = 0; j < network->outputLayer.numNeurons; j++)
        {
            dTotalErrorToHiddenNeuron +=
                dOutputErrorToOutputSum[j] * network->outputLayer.neuron[j]->params[i]->weight;
            // printf("\nOld Output Weight[%d][%d]: %.3f", i, j,
            // network->outputLayer.neuron[j]->params[i]->weight);
            network->outputLayer.neuron[j]->params[i]->weight += outputNeuronWeightChange[j][i];
            // printf("\nNew Output Weight[%d][%d]: %.3f", i, j,
            // network->outputLayer.neuron[j]->params[i]->weight);
        }
        for (uint16_t k = 0; k < network->inputLayer.numNeurons; k++)
        {
            // printf("\nOld Hidden Weight[%d][%d]: %.3f", i, k,
            // network->network->hiddenLayer[0].neuron[i]->params[k]->weight);
            network->hiddenLayer[0].neuron[i]->params[k]->weight +=
                dTotalErrorToHiddenNeuron *
                embann_tanhDerivative(network->hiddenLayer[0].neuron[i]->activation) *
                network->inputLayer.neuron[k]->activation * learningRate;
            // printf("\nNew Hidden Weight[%d][%d]: %.3f", i, k,
            // network->network->hiddenLayer[0].neuron[i]->params[k]->weight);
        }
    }
}

float embann_tanhDerivative(float inputValue)
{
    // if (inputValue < 0)
    //{
    //  return -1 * (1 - pow(tanh(inputValue), 2));
    //}
    // else
    //{
    return 1 - pow(tanh(inputValue * PI), 2);
    //}
}

void embann_printInputNeuronDetails(uint8_t neuronNum)
{
    if (neuronNum < network->inputLayer.numNeurons)
    {
        printf("\nInput Neuron %d: %.3f\n", neuronNum,
                      network->inputLayer.neuron[neuronNum]);
    }
    else
    {
        printf("\nERROR: You've asked for input neuron %d when only %d exist\n",
            neuronNum, network->inputLayer.numNeurons);
    }
}

void embann_printOutputNeuronDetails(uint8_t neuronNum)
{
    if (neuronNum < network->outputLayer.numNeurons)
    {

        printf("\nOutput Neuron %d:\n", neuronNum);

        for (uint16_t i = 0; i < network->hiddenLayer[0].numNeurons; i++)
        {
            printf(
                "%.3f-*->%.3f |",
                network->hiddenLayer[network->properties.numHiddenLayers - 1].neuron[i],
                network->outputLayer.neuron[neuronNum]->params[i]->weight);

            if (i == floor(network->hiddenLayer[0].numNeurons / 2))
            {
                printf(" = %.3f", network->outputLayer.neuron[neuronNum]);
            }
            printf("\n");
        }
    }
    else
    {
        printf(
            "\nERROR: You've asked for output neuron %d when only %d exist\n",
            neuronNum, network->outputLayer.numNeurons);
    }
}

void embann_printHiddenNeuronDetails(uint8_t layerNum, uint8_t neuronNum)
{
    if (neuronNum < network->hiddenLayer[0].numNeurons)
    {

        printf("\nHidden Neuron %d:\n", neuronNum);

        if (layerNum == 0)
        {

            for (uint16_t i = 0; i < network->inputLayer.numNeurons; i++)
            {
                printf("%.3f-*->%.3f |", network->inputLayer.neuron[i],
                              network->hiddenLayer[0].neuron[neuronNum]->params[i]->weight);

                if (i == floor(network->inputLayer.numNeurons / 2))
                {
                    printf(" = %.3f",
                                  network->hiddenLayer[0].neuron[neuronNum]);
                }
                printf("\n");
            }
        }
        else
        {

            for (uint16_t i = 0; i < network->hiddenLayer[0].numNeurons; i++)
            {
                printf(
                    "%.3f-*->%.3f |", network->hiddenLayer[layerNum - 1].neuron[i],
                    network->hiddenLayer[layerNum - 1].neuron[neuronNum]->params[i]->weight);

                if (i == floor(network->hiddenLayer[0].numNeurons / 2))
                {
                    printf(" = %.3f",
                                  network->hiddenLayer[0].neuron[neuronNum]);
                }
                printf("\n");
            }
        }
    }
    else
    {
        printf(
            "\nERROR: You've asked for hidden neuron %d when only %d exist\n",
            neuronNum, network->hiddenLayer[0].numNeurons);
    }
}

void embann_errorReporting(uint8_t correctResponse)
{
    printf("\n");
    for (uint8_t i = 0; i < network->outputLayer.numNeurons; i++)
    {
        if (i == correctResponse)
        {
            printf("%-7.3f | ",
                          (1 - network->outputLayer.neuron[correctResponse]->activation));
        }
        else
        {
            printf("%-7.3f | ", -network->outputLayer.neuron[i]->activation);
        }
    }
}

void embann_benchmark(void)
{
    uint32_t testint = UINT32_MAX;
    float testfloat = FLT_MAX;
    struct timespec timeBefore;
    struct timespec timeAfter;
    struct timespec averageTime = 
    {
        .tv_nsec = 0,
    };

    for (uint8_t i = 0; i < 5; i++)
    {
        clock_gettime(CLOCK_REALTIME, &timeBefore);

        for (uint32_t i = 0; i < INT32_MAX; i++)
        {
            testint /= 2;
            testint += 5;
        }

        clock_gettime(CLOCK_REALTIME, &timeAfter);

        averageTime.tv_nsec += timeAfter.tv_nsec - timeBefore.tv_nsec;
        printf("Integer time was %ul nanoseconds, result %ul", timeAfter.tv_nsec - timeBefore.tv_nsec, testint);
    }

    averageTime.tv_nsec /= 5;
    printf("Average integer time was %ul nanoseconds", averageTime.tv_nsec);
    averageTime.tv_nsec = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
        clock_gettime(CLOCK_REALTIME, &timeBefore);

        for (uint32_t i = 0; i < INT32_MAX; i++)
        {
            testfloat *= 0.5;
            testfloat += 5;
        }

        clock_gettime(CLOCK_REALTIME, &timeAfter);

        averageTime.tv_nsec += timeAfter.tv_nsec - timeBefore.tv_nsec;
        printf("Float time was %ul nanoseconds, result %.2f", timeAfter.tv_nsec - timeBefore.tv_nsec, testfloat);
    }

    averageTime.tv_nsec /= 5;
    printf("Average float time was %ul nanoseconds", averageTime.tv_nsec);
}

#ifndef ARDUINO
uint32_t millis(void)
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return (uint32_t) round(time.tv_nsec / 1000000);
}
#endif
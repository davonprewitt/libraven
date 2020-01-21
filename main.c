#include <libARSAL/ARSAL.h>
#include <libARController/ARController.h>
#include <libARDiscovery/ARDiscovery.h>

#include <semaphore.h>

#define DEVICE_NAME "Bebop2-084834"
#define BEBOP_IP_ADDRESS "192.168.42.1"
#define BEBOP_DISCOVERY_PORT 44444
#define TAG "ARDRONESDK"


ARDISCOVERY_Device_t* createDiscoveryDevice(eARDISCOVERY_PRODUCT product, const char *name, const char *ip, int port)
{
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Connecting to drone : ");
    
    // Init device
    ARDISCOVERY_Device_t *device = NULL;
    eARDISCOVERY_ERROR errorDiscovery = ARDISCOVERY_OK;
    
    if (ip == NULL || port == 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- Bad ip and/or port : ");
        return device;
    }
    
    // Discover device
    device = ARDISCOVERY_Device_New (&errorDiscovery);
    if (errorDiscovery == ARDISCOVERY_OK)
    {
        errorDiscovery =
            ARDISCOVERY_Device_InitWifi (device, product, name, ip, port);
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Discovered device : %p", device);
    
    }
    
    if (errorDiscovery != ARDISCOVERY_OK)
    {
        ARDISCOVERY_Device_Delete(&device);
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- Discovery error : ", ARDISCOVERY_Error_ToString(errorDiscovery));
    }

    return device;
}

// called when the state of the device controller has changed
void stateChanged (eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData)
{
    switch (newState)
    {
        case ARCONTROLLER_DEVICE_STATE_RUNNING:
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Device running : ");
            break;
        case ARCONTROLLER_DEVICE_STATE_STOPPED:
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Device stopped : ");
            break;
        case ARCONTROLLER_DEVICE_STATE_STARTING:
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Device starting : ");
            break;
        case ARCONTROLLER_DEVICE_STATE_STOPPING:
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Device %p stopping : ");
            break;
        default:
            break;
    }
}

void onCommandReceived (eARCONTROLLER_DICTIONARY_KEY commandKey, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary, void *customData)
{
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- 1 Controller error : ");
    if (elementDictionary != NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Debug Controller error : ");
        // if the command received is a battery state changed
        if (commandKey == ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED)
        {
            ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
            ARCONTROLLER_DICTIONARY_ELEMENT_t *element = NULL;

            // get the command received in the device controller
            HASH_FIND_STR (elementDictionary, ARCONTROLLER_DICTIONARY_SINGLE_KEY, element);
            if (element != NULL)
            {
                // get the value
                HASH_FIND_STR (element->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED_PERCENT, arg);

                if (arg != NULL)
                {
                    uint8_t batteryLevel = arg->value.U8;
                    // do what you want with the battery level
                    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Command received : ");
                }
            }
        }
        // else if (commandKey == THE COMMAND YOU ARE INTERESTED IN)
    }
}

// This function will wait until the device controller is stopped
void deleteDeviceController(ARCONTROLLER_Device_t *deviceController)
{
    if (deviceController == NULL)
    {
        return;
    }

    eARCONTROLLER_ERROR error = ARCONTROLLER_OK;

    eARCONTROLLER_DEVICE_STATE state = ARCONTROLLER_Device_GetState(deviceController, &error);
    if ((error == ARCONTROLLER_OK) && (state != ARCONTROLLER_DEVICE_STATE_STOPPED))
    {
        // after that, stateChanged should be called soon
        error = ARCONTROLLER_Device_Stop (deviceController);

        if (error == ARCONTROLLER_OK)
        {
//            sem_wait(&someSemaphore);
        }
        else
        {
            fprintf(stderr, "- error:%s", ARCONTROLLER_Error_ToString(error));
        }
    }

    // once the device controller is stopped, we can delete it
    ARCONTROLLER_Device_Delete(&deviceController);
}

eARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE getFlyingState(ARCONTROLLER_Device_t *deviceController)
{
    eARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE flyingState = ARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE_MAX;
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- FLYING %x", flyingState);
    eARCONTROLLER_ERROR error;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary = ARCONTROLLER_ARDrone3_GetCommandElements(deviceController->aRDrone3, ARCONTROLLER_DICTIONARY_KEY_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED, &error);
    if (error != ARCONTROLLER_OK) ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- FLYING %x", flyingState);
    if (error == ARCONTROLLER_OK && elementDictionary != NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- FLYING NULL", deviceController);
        ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
        ARCONTROLLER_DICTIONARY_ELEMENT_t *element = NULL;
        HASH_FIND_STR (elementDictionary, ARCONTROLLER_DICTIONARY_SINGLE_KEY, element);
        if (element != NULL)
        {
            // Get the value
            HASH_FIND_STR(element->arguments, ARCONTROLLER_DICTIONARY_KEY_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE, arg);
            if (arg != NULL)
            {
                // Enums are stored as I32
                flyingState = arg->value.I32;
            }
        }
    }
    return flyingState;
}

void takeOff(ARCONTROLLER_Device_t *deviceController)
{
    if (deviceController == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- DEVICE NULL", deviceController);
        return;
    }
    if (getFlyingState(deviceController) == ARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE_LANDED)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- DAMN 2", deviceController);
        deviceController->aRDrone3->sendPilotingTakeOff(deviceController->aRDrone3);
    }
    deviceController->aRDrone3->sendPilotingTakeOff(deviceController->aRDrone3);
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- DAMN", deviceController);
}

void land(ARCONTROLLER_Device_t *deviceController)
{
    if (deviceController == NULL)
    {
        return;
    }
    eARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE flyingState = getFlyingState(deviceController);
    if (flyingState == ARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE_FLYING || flyingState == ARCOMMANDS_ARDRONE3_PILOTINGSTATE_FLYINGSTATECHANGED_STATE_HOVERING)
    {
        deviceController->aRDrone3->sendPilotingLanding(deviceController->aRDrone3);
    }
}

int main()
{
    // Init device
    ARDISCOVERY_Device_t* device = createDiscoveryDevice(ARDISCOVERY_PRODUCT_BEBOP_2, DEVICE_NAME, BEBOP_IP_ADDRESS, BEBOP_DISCOVERY_PORT);
    if (device == NULL) return 0;
    
    // Init device controller
    eARCONTROLLER_ERROR errorController = ARCONTROLLER_OK;
    ARCONTROLLER_Device_t *deviceController = ARCONTROLLER_Device_New (device, &errorController);
    if (errorController == ARCONTROLLER_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Initialized device controller", device);
    } else
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- Discovery error : ", ARDISCOVERY_Error_ToString(errorController));
        return 0;
    }
    
    // Start error callbacks
    errorController = ARCONTROLLER_Device_AddStateChangedCallback(deviceController, stateChanged, NULL);
    if (errorController == ARCONTROLLER_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Handle 1", device);
    }
    errorController = ARCONTROLLER_Device_AddCommandReceivedCallback(deviceController, onCommandReceived, NULL);
    if (errorController == ARCONTROLLER_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Handle 2", device);
    }
    
    // Start device controller
    errorController = ARCONTROLLER_Device_Start (deviceController);
    if (errorController == ARCONTROLLER_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Started device controller", device);
    }
    errorController = deviceController->aRDrone3->sendPilotingTakeOff(deviceController->aRDrone3);
    if (errorController == ARCONTROLLER_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- Controller error : ");
    }
    takeOff(deviceController);
//    land(deviceController);
//
//    deviceController->aRDrone3->setPilotingPCMDFlag(deviceController->aRDrone3, 1);
//    deviceController->aRDrone3->setPilotingPCMDPitch(deviceController->aRDrone3, 50);
}

#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <libARSAL/ARSAL.h>
#include <libARController/ARController.h>
#include <libARDiscovery/ARDiscovery.h>

#define TAG "BebopSample"

#define ERROR_STR_LENGTH 2048

#define BEBOP_IP_ADDRESS "192.168.42.1"
#define BEBOP_DISCOVERY_PORT 44444

#define DISPLAY_WITH_MPLAYER 1

#define FIFO_DIR_PATTERN "/tmp/arsdk_XXXXXX"
#define FIFO_NAME "arsdk_fifo"



static char fifo_dir[] = FIFO_DIR_PATTERN;
static char fifo_name[128] = "";

int gIHMRun = 1;
char gErrorStr[ERROR_STR_LENGTH];
// IHM_t *ihm = NULL;

FILE *videoOut = NULL;
int frameNb = 0;
ARSAL_Sem_t stateSem;
int isBebop2 = 0;

static void signal_handler(int signal)
{
    gIHMRun = 0;
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

// called when the state of the device controller has changed
void stateChanged (eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData)
{
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "    - stateChanged newState: %d .....", newState);

    switch (newState)
    {
    case ARCONTROLLER_DEVICE_STATE_STOPPED:
        ARSAL_Sem_Post (&(stateSem));
        //stop
        gIHMRun = 0;

        break;

    case ARCONTROLLER_DEVICE_STATE_RUNNING:
        ARSAL_Sem_Post (&(stateSem));
        break;

    default:
        break;
    }
}

static void cmdBatteryStateChangedRcv(ARCONTROLLER_Device_t *deviceController, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary)
{
    ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *singleElement = NULL;

    if (elementDictionary == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "elements is NULL");
        return;
    }

    // get the command received in the device controller
    HASH_FIND_STR (elementDictionary, ARCONTROLLER_DICTIONARY_SINGLE_KEY, singleElement);

    if (singleElement == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "singleElement is NULL");
        return;
    }

    // get the value
    HASH_FIND_STR (singleElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED_PERCENT, arg);

    if (arg == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg is NULL");
        return;
    }

    // update UI
    // batteryStateChanged(arg->value.U8);
}

static void cmdSensorStateListChangedRcv(ARCONTROLLER_Device_t *deviceController, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary)
{
    ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *dictElement = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *dictTmp = NULL;

    eARCOMMANDS_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME sensorName = ARCOMMANDS_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME_MAX;
    int sensorState = 0;

    if (elementDictionary == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "elements is NULL");
        return;
    }

    // get the command received in the device controller
    HASH_ITER(hh, elementDictionary, dictElement, dictTmp) {
        // get the Name
        HASH_FIND_STR (dictElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME, arg);
        if (arg != NULL) {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg sensorName is NULL");
            continue;
        }

        sensorName = arg->value.I32;

        // get the state
        HASH_FIND_STR (dictElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORSTATE, arg);
        if (arg == NULL) {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg sensorState is NULL");
            continue;
        }

        sensorState = arg->value.U8;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "sensorName %d ; sensorState: %d", sensorName, sensorState);
    }
}

// called when a command has been received from the drone
void commandReceived (eARCONTROLLER_DICTIONARY_KEY commandKey, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary, void *customData)
{
    ARCONTROLLER_Device_t *deviceController = customData;

    if (deviceController == NULL)
        return;

    // if the command received is a battery state changed
    switch(commandKey) {
    case ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED:
        cmdBatteryStateChangedRcv(deviceController, elementDictionary);
        break;
    case ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED:
        cmdSensorStateListChangedRcv(deviceController, elementDictionary);
        break;
    default:
        break;
    }
}

void batteryStateChanged (uint8_t percent)
{
    // callback of changing of battery level

    // if (ihm != NULL)
    // {
    //     // IHM_PrintBattery (ihm, percent);
    // }
}

eARCONTROLLER_ERROR decoderConfigCallback (ARCONTROLLER_Stream_Codec_t codec, void *customData)
{
    if (videoOut != NULL)
    {
        if (codec.type == ARCONTROLLER_STREAM_CODEC_TYPE_H264)
        {
            if (DISPLAY_WITH_MPLAYER)
            {
                fwrite(codec.parameters.h264parameters.spsBuffer, codec.parameters.h264parameters.spsSize, 1, videoOut);
                fwrite(codec.parameters.h264parameters.ppsBuffer, codec.parameters.h264parameters.ppsSize, 1, videoOut);

                fflush (videoOut);
            }
        }

    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "videoOut is NULL.");
    }

    return ARCONTROLLER_OK;
}


eARCONTROLLER_ERROR didReceiveFrameCallback (ARCONTROLLER_Frame_t *frame, void *customData)
{
    if (videoOut != NULL)
    {
        if (frame != NULL)
        {
            if (DISPLAY_WITH_MPLAYER)
            {
                fwrite(frame->data, frame->used, 1, videoOut);

                fflush (videoOut);
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "frame is NULL.");
        }
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "videoOut is NULL.");
    }

    return ARCONTROLLER_OK;
}


// IHM callbacks:



int customPrintCallback (eARSAL_PRINT_LEVEL level, const char *tag, const char *format, va_list va)
{
    // Custom callback used when ncurses is runing for not disturb the IHM

    if ((level == ARSAL_PRINT_ERROR) && (strcmp(TAG, tag) == 0))
    {
        // Save the last Error
        vsnprintf(gErrorStr, (ERROR_STR_LENGTH - 1), format, va);
        gErrorStr[ERROR_STR_LENGTH - 1] = '\0';
    }

    return 1;
}
int main (int argc, char *argv[])
{
    // local declarations
    int failed = 0;
    ARDISCOVERY_Device_t *device = NULL;
    ARCONTROLLER_Device_t *deviceController = NULL;
    eARCONTROLLER_ERROR error = ARCONTROLLER_OK;
    eARCONTROLLER_DEVICE_STATE deviceState = ARCONTROLLER_DEVICE_STATE_MAX;
    pid_t child = 0;

    /* Set signal handlers */
    struct sigaction sig_action = {
        .sa_handler = signal_handler,
    };

    int ret = sigaction(SIGINT, &sig_action, NULL);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Unable to set SIGINT handler : %d(%s)",
                    errno, strerror(errno));
        return 1;
    }
    ret = sigaction(SIGPIPE, &sig_action, NULL);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Unable to set SIGPIPE handler : %d(%s)",
                    errno, strerror(errno));
        return 1;
    }


    if (mkdtemp(fifo_dir) == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Mkdtemp failed.");
        return 1;
    }
    snprintf(fifo_name, sizeof(fifo_name), "%s/%s", fifo_dir, FIFO_NAME);

    if(mkfifo(fifo_name, 0666) < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Mkfifo failed: %d, %s", errno, strerror(errno));
        return 1;
    }

    ARSAL_Sem_Init (&(stateSem), 0, 0);

    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Select your Bebop : Bebop (1) ; Bebop2 (2)");
    char answer = '1';
    scanf(" %c", &answer);
    
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "-- Bebop 2 Sample --");


    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- init discovey device ... ");
    // create a discovery device
    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- init discovey device ... ");
        eARDISCOVERY_ERROR errorDiscovery = ARDISCOVERY_OK;

        device = ARDISCOVERY_Device_New (&errorDiscovery);

        if (errorDiscovery == ARDISCOVERY_OK)
        {
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "    - ARDISCOVERY_Device_InitWifi ...");
            // create a Bebop drone discovery device (ARDISCOVERY_PRODUCT_ARDRONE)

            errorDiscovery = ARDISCOVERY_Device_InitWifi (device, ARDISCOVERY_PRODUCT_BEBOP_2, "Bebop2-084834", BEBOP_IP_ADDRESS, BEBOP_DISCOVERY_PORT);

            if (errorDiscovery != ARDISCOVERY_OK)
            {
                failed = 1;
                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Discovery error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Discovery error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
            failed = 1;
        }
    }

    // create a device controller
    if (!failed)
    {
        deviceController = ARCONTROLLER_Device_New (device, &error);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "Creation of deviceController failed.");
            failed = 1;
        }
        else
        {
            // IHM_setCustomData(ihm, deviceController);
        }
    }

    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- delete discovey device ... ");
        ARDISCOVERY_Device_Delete (&device);
    }

    // add the state change callback to be informed when the device controller starts, stops...
    if (!failed)
    {
        error = ARCONTROLLER_Device_AddStateChangedCallback (deviceController, stateChanged, deviceController);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add State callback failed.");
            failed = 1;
        }
    }

    // add the command received callback to be informed when a command has been received from the device
    if (!failed)
    {
        error = ARCONTROLLER_Device_AddCommandReceivedCallback (deviceController, commandReceived, deviceController);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add callback failed.");
            failed = 1;
        }
    }

    // add the frame received callback to be informed when a streaming frame has been received from the device
    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- set Video callback ... ");
        // error = ARCONTROLLER_Device_SetVideoStreamCallbacks (deviceController, decoderConfigCallback, didReceiveFrameCallback, NULL , NULL);

        // if (error != ARCONTROLLER_OK)
        // {
        //     failed = 1;
        //     ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error: %s", ARCONTROLLER_Error_ToString(error));
        // }
    }

    if (!failed)
    {
        // IHM_PrintInfo(ihm, "Connecting ...");
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Connecting ...");
        error = ARCONTROLLER_Device_Start (deviceController);

        if (error != ARCONTROLLER_OK)
        {
            failed = 1;
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- not this error :%s", ARCONTROLLER_Error_ToString(error));
        }
    }

    if (!failed)
    {
        // wait state update update
        ARSAL_Sem_Wait (&(stateSem));

        deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);

        if ((error != ARCONTROLLER_OK) || (deviceState != ARCONTROLLER_DEVICE_STATE_RUNNING))
        {
            failed = 1;
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- deviceState :%d", deviceState);
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
        }
        takeOff(deviceController);
    }
/// ***********
    // send the command that tells to the Bebop to begin its streaming
    // if (!failed)
    // {
    //     ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- send StreamingVideoEnable ... ");
    //     error = deviceController->aRDrone3->sendMediaStreamingVideoEnable (deviceController->aRDrone3, 1);
    //     if (error != ARCONTROLLER_OK)
    //     {
    //         ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
    //         failed = 1;
    //     }
    // }

    if (!failed)
    {
        // IHM_PrintInfo(ihm, "Running ... ('t' to takeoff ; Spacebar to land ; 'e' for emergency ; Arrow keys and ('r','f','d','g') to move ; 'q' to quit)");

#ifdef IHM
        while (gIHMRun)
        {
            usleep(50);
        }
#else
        int i = 20;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- sleep 20 ... ");
        while (gIHMRun && i--)
            sleep(1);
#endif
    }

#ifdef IHM
    IHM_Delete (&ihm);
#endif

    // we are here because of a disconnection or user has quit IHM, so safely delete everything
    if (deviceController != NULL)
    {


        deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);
        if ((error == ARCONTROLLER_OK) && (deviceState != ARCONTROLLER_DEVICE_STATE_STOPPED))
        {
            // IHM_PrintInfo(ihm, "Disconnecting ...");
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Disconnecting ...");

            error = ARCONTROLLER_Device_Stop (deviceController);

            if (error == ARCONTROLLER_OK)
            {
                // wait state update update
                ARSAL_Sem_Wait (&(stateSem));
            }
        }

        // IHM_PrintInfo(ihm, "");
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "ARCONTROLLER_Device_Delete ...");
        ARCONTROLLER_Device_Delete (&deviceController);

        if (DISPLAY_WITH_MPLAYER)
        {
            fflush (videoOut);
            fclose (videoOut);

            if (child > 0)
            {
                kill(child, SIGKILL);
            }
        }
    }

    ARSAL_Sem_Destroy (&(stateSem));

    unlink(fifo_name);
    rmdir(fifo_dir);

    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "-- END --");

    return EXIT_SUCCESS;
}

/*****************************************
 *
 *             private implementation:
 *
 ****************************************/


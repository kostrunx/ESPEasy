#ifdef USES_C008
//#######################################################################################################
//########################### Controller Plugin 008: Generic HTTP #######################################
//#######################################################################################################

#define CPLUGIN_008
#define CPLUGIN_ID_008         8
#define CPLUGIN_NAME_008       "Generic HTTP"
#include <ArduinoJson.h>

boolean CPlugin_008(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_008;
        Protocol[protocolCount].usesMQTT = false;
        Protocol[protocolCount].usesTemplate = true;
        Protocol[protocolCount].usesAccount = true;
        Protocol[protocolCount].usesPassword = true;
        Protocol[protocolCount].defaultPort = 80;
        Protocol[protocolCount].usesID = true;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_008);
        break;
      }

    case CPLUGIN_INIT:
      {
        ControllerSettingsStruct ControllerSettings;
        LoadControllerSettings(event->ControllerIndex, (byte*)&ControllerSettings, sizeof(ControllerSettings));
        C008_DelayHandler.configureControllerSettings(ControllerSettings);
        break;
      }

    case CPLUGIN_PROTOCOL_TEMPLATE:
      {
        event->String1 = "";
        event->String2 = F("demo.php?name=%sysname%&task=%tskname%&valuename=%valname%&value=%value%");
        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        // Collect the values at the same run, to make sure all are from the same sample
        byte valueCount = getValueCountFromSensorType(event->sensorType);
        C008_queue_element element(event, valueCount);
        if (ExtraTaskSettings.TaskDeviceValueNames[0][0] == 0)
          PluginCall(PLUGIN_GET_DEVICEVALUENAMES, event, dummyString);

        ControllerSettingsStruct ControllerSettings;
        LoadControllerSettings(event->ControllerIndex, (byte*)&ControllerSettings, sizeof(ControllerSettings));

        for (byte x = 0; x < valueCount; x++)
        {
          bool isvalid;
          String formattedValue = formatUserVar(event, x, isvalid);
          if (isvalid) {
            element.url[x] = "/";
            element.url[x] += ControllerSettings.Publish;
            parseControllerVariables(element.url[x], event, true);

            element.url[x].replace(F("%valname%"), URLEncode(ExtraTaskSettings.TaskDeviceValueNames[x]));
            element.url[x].replace(F("%value%"), formattedValue);
            addLog(LOG_LEVEL_DEBUG_MORE, element.url[x]);
          }
        }
        success = C008_DelayHandler.addToQueue(element);
        scheduleNextDelayQueue(TIMER_C008_DELAY_QUEUE, C008_DelayHandler.getNextScheduleTime());
        break;
      }

  }
  return success;
}

//********************************************************************************
// Generic HTTP get request
//********************************************************************************
bool do_process_c008_delay_queue(const C008_queue_element& element, ControllerSettingsStruct& ControllerSettings) {
  while (element.url[element.valuesSent] == "") {
    // A non valid value, which we are not going to send.
    // Increase sent counter until a valid value is found.
    if (element.checkDone(true))
      return true;
  }

  WiFiClient client;
  if (!try_connect_host(CPLUGIN_ID_008, client, ControllerSettings))
    return false;

  String request = create_http_request_auth(CPLUGIN_ID_008, ControllerSettings, F("GET"), element.url[element.valuesSent]);
  return element.checkDone(send_via_http(CPLUGIN_ID_008, client, request));
}

#endif

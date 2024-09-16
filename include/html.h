#pragma once

#define FORM_SAVE_STRING(VAR)                                  \
  strncpy(eeprom.VAR,                                          \
          request->hasParam(#VAR, true)                        \
              ? request->getParam(#VAR, true)->value().c_str() \
              : "",                                            \
          sizeof(eeprom.VAR));
#define FORM_SAVE_INT(VAR)                                          \
  eeprom.VAR = request->hasParam(#VAR, true)                        \
                   ? request->getParam(#VAR, true)->value().toInt() \
                   : 0;
#define FORM_SAVE_BOOL(VAR)                                                 \
  eeprom.VAR =                                                              \
      request->hasParam(#VAR, true)                                         \
          ? (request->getParam(#VAR, true)->value() == "on" ? true : false) \
          : false;

#define FORM_START(URL) \
  s += "<form action='" + String(URL) + "' method='POST'>";
#define FORM_ASK_VALUE(VAR, TXT)                                           \
  s += "<label for='" + String(#VAR) + "' name='" + String(#VAR) + "'>" +  \
       String(TXT) + ":</label><input type='text' name='" + String(#VAR) + \
       "' value='" + eeprom.VAR + "'><br>";
#define FORM_ASK_BOOL(VAR, TXT)                                                \
  s += "<label for='" + String(#VAR) + "' name='" + String(#VAR) + "'>" +      \
       String(TXT) + ":</label><input type='checkbox' name='" + String(#VAR) + \
       "' " + String(eeprom.VAR ? "checked" : "") + "><br>";
#define FORM_ASK_BOOL_JS(VAR, TXT, JS)                                         \
  s += "<label for='" + String(#VAR) + "' name='" + String(#VAR) + "'>" +      \
       String(TXT) + ":</label><input type='checkbox' name='" + String(#VAR) + \
       "' " + String(eeprom.VAR ? "checked " : "") + String(JS) + "><br>";
#define FORM_END(BTN)                                                          \
  s +=                                                                         \
      "<input type='hidden' name='s' value='1'><input type='submit' value='" + \
      String(BTN) + "'></form>";

const char html_header[] /*PROGMEM*/ =
    R""""(<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
<meta http-equiv='cache-control' content='no-cache, no-store, must-revalidate'><meta http-equiv='refresh' content='600'/><title>MQTT SERVER</title></head><body>)"""";

const char html_footer[] /*PROGMEM*/ = R""""(</body></html>)"""";

const char js_mqtt_remote_enable[] /*PROGMEM*/ =
    R""""(onclick="_enable_disable('mqtt_remote_enable',mqtt_remote_group)")"""";

const char js_config[] /*PROGMEM*/ = R""""(<script>
function _enable_disable(cbox, elements){
var status=!(document.getElementsByName(cbox)[1].checked);
  for (const element of elements) {
    var e=document.getElementsByName(element);
    e[0].hidden=status;
    e[1].hidden=status;
  }
}

mqtt_remote_group=['mqtt_remote_ip', 'mqtt_remote_port', 'mqtt_remote_username', 'mqtt_remote_password', 'mqtt_remote_send', 'mqtt_remote_receive','mqtt_remote_remove_prefix','mqtt_remote_add_prefix'];
_enable_disable('mqtt_remote_enable',mqtt_remote_group);
</script>
)"""";

#pragma once

const char html_header[] PROGMEM =
    R""""(<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
<meta http-equiv='cache-control' content='no-cache, no-store, must-revalidate'><meta http-equiv='refresh' content='600'/><title>MQTT SERVER</title></head><body>)"""";

const char html_footer[] PROGMEM = R""""(</body></html>)"""";

const char js_mqtt_remote_enable[] PROGMEM =
    R""""(onclick=_enable_disable_remote_mqtt())"""";

const char js_config[] PROGMEM = R""""(
<script>
function _enable_disable_remote_mqtt(){
  var status=!(document.getElementsByName('mqtt_remote_enable')[1].checked);
  const elements = ['mqtt_remote_ip', 'mqtt_remote_port', 'mqtt_remote_username', 'mqtt_remote_password', 'mqtt_remote_send', 'mqtt_remote_receive'];
  for (const element of elements) {
    var e = document.getElementsByName(element);
    for(var i of e) {
      i.hidden=status;
    }
  }
}
_enable_disable_remote_mqtt();
</script>
)"""";

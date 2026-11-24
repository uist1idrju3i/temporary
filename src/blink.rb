led = Led.new(0)

while true
  led.on
  sleep_ms 150
  led.off
  sleep_ms 150
end

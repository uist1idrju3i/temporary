puts "mruby/c on nRF54L15-DK"

count = 0
while true
  count += 1
  puts "heartbeat #{count}"
  sleep_ms 1000
end

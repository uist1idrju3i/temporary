# Zephyr HAL verification test for mruby/c
# Checks: output via mrbc_hal_write (printk), VM tick, basic arithmetic,
#         string operations, and scheduler exit.

puts "mruby/c Zephyr HAL test start"

# Basic arithmetic
a = 6
b = 7
puts a * b

# Float arithmetic
x = 1.5
y = 2.0
puts x + y

# String
s = "hello"
puts s + " zephyr"

# Conditional
if a > 0
  puts "positive"
end

# Loop with counter
i = 0
while i < 3
  puts i
  i = i + 1
end

# VM tick (uptime counter increments during sleep)
t0 = VM.tick
sleep_ms 10
t1 = VM.tick
if t1 > t0
  puts "tick ok"
else
  puts "tick ng"
end

puts "mruby/c Zephyr HAL test done"

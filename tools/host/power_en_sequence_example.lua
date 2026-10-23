-- Requires MCU firmware with A0 2B and updated gcv5_service HAL.
-- Configure rail voltages/current limits for your panel before executing.
-- time_us, VCC, IOVCC, VSP, VSN, AVDD, VDD, ELVDD, ELVSS
local status = power.set_en_sequence({
    {0,    1,1,0,0,0,0,0,0},
    {50,   1,1,1,0,0,0,0,0},
    {150,  1,1,1,0,0,0,1,0},
    {500,  1,1,1,0,1,0,1,0},
    {1000, 1,1,1,0,1,0,1,1},
})
assert(status == 0, 'Power-on EN sequence failed')
breakpoint()
status = power.set_en_sequence({
    {0,    1,1,1,0,1,0,1,0},
    {50,   1,1,1,0,0,0,1,0},
    {150,  1,1,1,0,0,0,0,0},
    {300,  1,1,0,0,0,0,0,0},
    {500,  0,1,0,0,0,0,0,0},
    {1000, 0,0,0,0,0,0,0,0},
})
assert(status == 0, 'Power-off EN sequence failed')

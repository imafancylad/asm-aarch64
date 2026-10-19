.global _start
.global entry_point

_start:
entry_point:
    mov x0, #10
    mov x1, #20
    add x2, x0, x1

local_label:
    mov x3, #30
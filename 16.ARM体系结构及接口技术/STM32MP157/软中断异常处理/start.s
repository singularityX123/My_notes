/*start.s*/
.text 
.global _start
_start:>
    b reset
    b undef_handler
    b swi_handler
    b pref_handler
    b data_handler
    b .
    b irq_handler
    b fiq_handler
    
reset:
    ldr sp, =svc_stack
    add sp, sp, #256
    
    mrs r0, cpsr
    bic r0, r0, #0x1f
    orr r0, r0, #0x10
    msr cpsr, r0
    
    ldr sp, =irq_stack
    add sp, sp, #256
    mov r0, #3
    mov r1, #4
    swi  #2  
    
    add r2, r0, r1  
    b stop

undef_handler:
    b stop

swi_handler:
    stmfd sp!, {r0-r1, lr}    
    mov r0, #5
    mov r1, #6
    
    ldmfd sp!, {r0-r1, pc}^ 

pref_handler:
    b stop

data_handler:
    b stop

irq_handler:
    b stop

fiq_handler:
    b stop

stop:   
    b stop  

.data 
svc_stack:
    .space 256

irq_stack:
    .space 256
    
.end

@ 观察异常产生后硬件自动做的事

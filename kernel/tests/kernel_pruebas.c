#include <kernel.h>
#include <cspecs/cspec.h>

context (probando_kernel) {
    describe("Inicialización") {
        before {
            printf("Yo inicializo cosas\n");
        } end

        after {
            printf("Yo limpio cosas\n");
        } end

        t_pcb* pcb;
        t_tcb* tcb;
        t_hilo_planificacion* hilo;

        describe("Creación de estructuras") {
            it("Creacion de proceso con hilo main") {
                pcb = create_pcb("P1.txt", 10);
                tcb = create_tcb(pcb, 0);
                
                should_int(pcb->pid) be equal to(0);

            } end

            it("test2") {
                printf("Soy el test 2 y doy Segmentation Fault\n");
                char* puntero = NULL;
                *puntero = 9;
            } end

            it("test3") {
                printf("Soy el test 3");
            } end
        } end
    } end
}


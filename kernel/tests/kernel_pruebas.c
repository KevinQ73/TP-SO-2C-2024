#include <kernel.h>
#include <cspecs/cspec.h>

context (probando_cosas) {
    describe("tests") {
        before {
            printf("Yo inicializo cosas\n");
        } end

        after {
            printf("Yo limpio cosas\n");
        } end

        it("test1") {
            printf("Soy el test 1 y pruebo que 1+1 sea 2\n");
            should_int(1 + 1) be equal to(2);
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
}


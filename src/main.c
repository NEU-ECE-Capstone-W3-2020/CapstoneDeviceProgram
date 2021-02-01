#define UNUSED(arg) (void) arg

// configure memory mapped periphirals
int init(void) {
    return 0;
}

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    if(init() != 0) {
        // Setup Failed
        return 1;
    }

    // Do 1 time operations

    while(1) {
        // main event loop
    }
}

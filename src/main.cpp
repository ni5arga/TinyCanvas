#include "editor.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int canvasW = 32, canvasH = 32;

    if (argc >= 3) {
        canvasW = atoi(argv[1]);
        canvasH = atoi(argv[2]);
        if (canvasW < 1 || canvasW > 512) canvasW = 32;
        if (canvasH < 1 || canvasH > 512) canvasH = 32;
    }

    printf("TinyCanvas: %dx%d\n", canvasW, canvasH);
    printf("Controls:\n");
    printf("  P/E/L/R/C/F/I  - Select tool\n");
    printf("  G               - Toggle grid\n");
    printf("  X               - Swap FG/BG colors\n");
    printf("  +/-             - Zoom in/out\n");
    printf("  Scroll wheel    - Zoom\n");
    printf("  Right-drag      - Pan\n");
    printf("  Cmd+Z / Cmd+Shift+Z - Undo/Redo\n");
    printf("  Cmd+S           - Save BMP\n");
    printf("  Cmd+N           - New canvas\n");

    Editor editor(canvasW, canvasH);
    if (!editor.init()) {
        fprintf(stderr, "Failed to initialize editor\n");
        return 1;
    }

    editor.run();
    return 0;
}

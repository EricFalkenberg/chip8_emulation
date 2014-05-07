#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <GLUT/glut.h>

#define TRUE 1
#define FALSE 0

void init_chip() {
    //
    //  Initializes the emulator
    //
    I = 0;                              // index register
    pc = 0x200;                         // pc starts at 0x200
    sp = 0;                             // stack pointer
    opcode = 0;                         // opcode
    delay_timer = 0;                    // delay and sound
    sound_timer = 0;
    for (int i=0;i<16;i++) {
        stack[i] = 0;                  // stack init
    }
    for (int i=0;i<4096;i++){
        memory[i] = 0;                  // mem init
    }
    for (int i=0;i<80;i++) {
        memory[i] = chip8_fontset[i];   // load fontset into memory
    }
    for (int i=0;i<16;i++) {
        v[i] = 0;                       // zero out registers
    }
    for (int i=0;i<16;i++) {
        key[i] = 0;                     // zero out keys
    }
    for (int i=0;i<2048;i++) {
        gfx[i] = 0;                     // init graphics array
    }
}

int loadProgram(char* fname) {
    //
    // Load program from file
    //
    FILE* file = fopen(fname,"r");
    if (file != NULL) {
        unsigned short* read = malloc(sizeof(char));
        int count = 512;
        while (fread(read, sizeof(char), 1, file)) {
            memory[count] = *read;
            count++;
        }
        free(read);
        return 1;
    }
    return 0;
}

void executeCycle() {
    //
    // Function of which will execute a single cycle for the emuator
    //
    opcode = memory[pc] << 8 | memory[pc+1];
    unsigned short vx, vy, height, pixel;
    switch(opcode & 0xF000) {
        case 0x0000:
            switch(opcode & 0x000F) {
                case 0x0000:
                //
                //  Clears the screen
                //                    
                    for (int i=0;i<2048;i++) {
                        gfx[i] = 0;
                    }
                    pc += 2;
                    break;
                case 0x000E:         
                //
                // Returns from a subroutine
                //           
                    --sp;
                    pc = stack[sp];
                    pc += 2;
                    break;
            }
            break;
        case 0x1000:
        //
        //  Jumps to address NNN
        //
            pc = opcode & 0x0FFF;            
            break;
        case 0x2000:            
        //
        //  Calls subroutine ad NNN
        //                
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:               
        //
        //  Skips the next instruction if VX equals NN
        //             
            if (v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        
        case 0x4000:
        //
        //  Skips the next instruction if VX doesnt equal NN
        //
            if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0x5000:
            switch (opcode & 0x000F) {
                case 0x0000:
                //
                //  Skips the next instruction if VX equals VY
                //
                    if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
            }
            break;
        case 0x6000:
        //
        //  Sets VX to NN
        //
            v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000:
        //
        //  Adds NN to VX
        //
            v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                //
                //  Sets VX to the value of VY
                //
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0001:
                //
                //  Sets VX to VX or VY
                //
                    vx = (opcode & 0x0F00) >> 8;
                    vy = (opcode & 0x00F0) >> 4;
                    v[vx] = v[vx] | v[vy];
                    pc += 2;
                    break;
                case 0x0002:
                //
                //  Sets VX to VX and VY
                //
                    vx = (opcode & 0x0F00) >> 8;
                    vy = (opcode & 0x00F0) >> 4;
                    v[vx] = v[vx] & v[vy];
                    pc += 2;
                    break;
                case 0x0003:
                //
                //  Sets VX to VX xor VY
                //
                    vx = (opcode & 0x0F00) >> 8;
                    vy = (opcode & 0x00F0) >> 4;
                    v[vx] = v[vx] ^ v[vy];
                    pc += 2;
                    break;
                case 0x0004:
                //
                //  Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when 
                //  there isnt
                //
                    if (v[(opcode & 0x00F0) >> 4] > (0xFF - v[(opcode & 0x0F00) >> 8])) {
                        v[0xF] = 1;
                    }
                    else {
                        v[0xF] = 0;
                    }
                    v[(opcode & 0x0F00) >> 8] += v[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0005:
                //
                //  VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when
                //  there isn't
                //
                    if (v[(opcode & 0x00F0) >> 4] > v[(opcode & 0x0F00) >> 8]) {
                        v[0xF] = 0;
                    }
                    else {
                        v[0xF] = 1;
                    }
                    v[(opcode & 0x0F00) >> 8] -= v[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0006:
                //
                //  Shifts VX right by one. VF is set to the value of the least significant
                //  bit of VX before the shift
                //
                    v[0xF] = v[(opcode & 0x0F00) >> 8] & 0x0001;
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x0F00) >> 8] >> 1;
                    pc += 2;
                    break;
                case 0x0007:
                //
                //  Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when
                //  there isn't
                //
                    if (v[(opcode & 0x0F00) >> 8] > v[(opcode & 0x00F0) >> 4]) {
                        v[0xF] = 0;
                    }
                    else {
                        v[0xF] = 1;
                    }
                    v[(opcode & 0x00F0) >> 4] -= v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x000E:
                //
                //  Shifts VX left by one. VF is set to the value of the most significant bit
                //  of VX before the shift
                //
                    v[0xF] = v[(opcode & 0x0F00) >> 8] & 0x8000;
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x0F00) >> 8] << 1;
                    pc += 2;
                    break;
            }
            break;
        case 0x9000:
        //
        //  Skips the next instruction if VX doesn't equal VY
        //
            if (v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4]) {
                pc += 2;
            }
            pc += 2;
            break;
        case 0xA000:
        //
        //  Sets I to the address NNN
        //
            I = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xB000:
        //
        //  Jumps to the address NNN plus V0
        //
            pc = (opcode & 0x0FFF) + v[0];
            break;
        case 0xC000:
        //
        //  Sets VX to a random number and NN
        //
            v[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
            break;
        case 0xD000:
        //
        //  Draws a sprite at coordinate (VX,VY) that has a width of 8 pixels and a height of N
        //  pixels. Each row of 8 pixels is read as bit-coded (with the most significant bit of
        //  each byte displayed on the left) starting from memory location I; I value doesn't
        //  change after the execution of this instruction. As described above, VF is set to 1
        //  if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0
        //  if that doesn't happen.
        //
            vx = v[(opcode & 0x0F00) >> 8];
            vy = v[(opcode & 0x00F0) >> 4];
            height = opcode & 0x000F;

            v[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if((pixel & (0x80 >> xline)) != 0) {
                        if (gfx[(vx + xline + ((vy + yline) * 64))] == 1) {
                            v[0xF] = 1;
                        }
                        gfx[vx + xline + ((vy + yline) * 64)] ^= 1;
                    }
                }
            }            
            drawFlag = TRUE;
            pc += 2;
            break;
        case 0xE000:
            switch (opcode & 0x000F) {
                case 0x000E:
                //
                //  Skips the next instruction if the key stored in VX is pressed.
                //
                    if (key[v[(opcode & 0x0F00) >> 8]] != 0) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
                case 0x0001:
                //
                //  Skips the next instruction if the key stored in VX isn't pressed.
                //
                    if (key[v[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 2;
                    }
                    pc += 2;
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007:
                //
                //  Sets VX to the value of the delay timer
                //
                    v[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A:
                //
                //  A key press is awaited, and then stored in VX
                //  TODO
                //
                    for (int i = 0; i < 16; i++) {
                        if (key[i] == 1) {
                            pc += 2;
                            break;
                        }
                    }
                    break;
                case 0x0015:
                //
                //  Sets the delay timer to VX
                //
                    delay_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0018:
                //
                //  Sets the sound timer to VX
                //
                    sound_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x001E:
                //
                //  Adds VX to I
                //
                    I += v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0029:
                //
                //  Sets I to the location of the sprite for the character in VX.
                //  Characters 0-F (in hexadecimal_ are represented by a 4x5 font.
                //
                    I = (5*v[(opcode & 0x0F00) >> 8]);
                    pc += 2;
                    break;
                case 0x0033:
                //
                //  Stores the Binary-coded decimal representation of VX, with the most
                //  significant of three digits at the address in I, the middle digit at I 
                //  plus 1, and the least significant digit at I plus 2. (In other words,
                //  take the decimal representation of VX, place the hundreds digit in memory
                //  at location in I, the tens digit at location I+1, and the ones digit at
                //  location I+2.)
                //
                    memory[I]     =  v[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (v[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (v[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;
                    break;
                case 0x0055:
                //
                //  Stores V0 to VX in memory starting at address I
                //
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                        memory[I+i] = v[i];
                    }
                    pc += 2;
                    break;
                case 0x0065:
                //
                //  Fills V0 to VX with values from memory starting at address I
                //
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                        v[i] = memory[I+i];
                    }
                    pc += 2;
                    break;
            }
            break;
        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            pc += 2;
            break;
    }    

    if (delay_timer > 0) {
        delay_timer--;
    }
    if (sound_timer > 0) {
        if (sound_timer == 1) {
            printf("boop\n");
        }
        sound_timer--;
    }
}

void drawScreen(void) {
    //
    // Function that will be called once per cycle.
    // if drawFlag is set to true, the screen will be redrawn
    // and another cycle executed.
    // if drawFlag is set to false, a cycle will still be executed
    // but the screen will not be drawn to.
    //
    float xline;
    float yline;
    int index = 0;
    if (drawFlag == TRUE) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 32; i++) {
            yline = i*0.0625f;
            for (int j = 0; j < 64; j++) {
                if (gfx[index] == 1) {
                    xline = j*0.03125f;
                    glBegin(GL_QUADS);
                        glVertex3f(-1.0f+xline,1.0f-yline,0.0f);
                        glVertex3f(-1.0f+xline,0.9375f-yline,0.0f);
                        glVertex3f(-0.96875f+xline,0.9375f-yline,0.0f);
                        glVertex3f(-0.96875f+xline,1.0f-yline,0.0f);
                    glEnd();
                }
                index++;
            }
        }
        glutSwapBuffers();
    }
    drawFlag = FALSE;
    executeCycle();
    usleep(2000);
    glutPostRedisplay();
}

void handleInput(unsigned char keyPressed, int x, int y) {
    if (keyPressed == '1') {key[0x1] = 1;}
    else if (keyPressed == '2') {key[0x2] = 1;}
    else if (keyPressed == '3') {key[0x3] = 1;}
    else if (keyPressed == '4') {key[0xC] = 1;}

    else if (keyPressed == 'q') {key[0x4] = 1;}
    else if (keyPressed == 'w') {key[0x5] = 1;}
    else if (keyPressed == 'e') {key[0x6] = 1;}
    else if (keyPressed == 'r') {key[0xD] = 1;}

    else if (keyPressed == 'a') {key[0x7] = 1;}
    else if (keyPressed == 's') {key[0x8] = 1;}
    else if (keyPressed == 'd') {key[0x9] = 1;}
    else if (keyPressed == 'f') {key[0xE] = 1;}
    
    else if (keyPressed == 'z') {key[0xA] = 1;}
    else if (keyPressed == 'x') {key[0x0] = 1;}
    else if (keyPressed == 'c') {key[0xB] = 1;}
    else if (keyPressed == 'v') {key[0xF] = 1;}
    
    glutPostRedisplay();
}

void handleInputRel(unsigned char keyPressed, int x, int y) {
    if (keyPressed == '1') {key[0x1] = 0;}
    else if (keyPressed == '2') {key[0x2] = 0;}
    else if (keyPressed == '3') {key[0x3] = 0;}
    else if (keyPressed == '4') {key[0xC] = 0;}

    else if (keyPressed == 'q') {key[0x4] = 0;}
    else if (keyPressed == 'w') {key[0x5] = 0;}
    else if (keyPressed == 'e') {key[0x6] = 0;}
    else if (keyPressed == 'r') {key[0xD] = 0;}

    else if (keyPressed == 'a') {key[0x7] = 0;}
    else if (keyPressed == 's') {key[0x8] = 0;}
    else if (keyPressed == 'd') {key[0x9] = 0;}
    else if (keyPressed == 'f') {key[0xE] = 0;}
    
    else if (keyPressed == 'z') {key[0xA] = 0;}
    else if (keyPressed == 'x') {key[0x0] = 0;}
    else if (keyPressed == 'c') {key[0xB] = 0;}
    else if (keyPressed == 'v') {key[0xF] = 0;}
    
    glutPostRedisplay();
}

int main(int argc, char **argv) {
    if (argc == 2) {
        init_chip();
        int fileExists = loadProgram(argv[1]);
        if (fileExists) {
            drawFlag = FALSE;
            glutInit(&argc, argv);
            glutInitDisplayMode(GLUT_RGB);
            glutInitWindowSize(640,320);
            glutCreateWindow("Emulator");
            glutDisplayFunc(drawScreen);
            glutKeyboardFunc(handleInput);
            glutKeyboardUpFunc(handleInputRel);
            glutMainLoop();
        }
        else {
            printf("File does not exist.\n");
        }
    }
    else {
        printf("usage: emu8 <filename>\n");
    }
    return EXIT_SUCCESS;
}


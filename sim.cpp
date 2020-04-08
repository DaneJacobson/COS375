#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <string>
#include "MemoryStore.h"
#include "RegisterInfo.h"
#include "EndianHelpers.h"

#define r_SIZE 32

using namespace std;

int main(int argc, char *argv[])
{
  // creates memory abstraction
  MemoryStore *myMem = createMemoryStore();
  uint32_t r[r_SIZE] = {0};

  // variables for reading bytes into memory
  char buffer[4];
  uint32_t address = 0x0;
  int size;
  uint32_t num;

  // variables for opcode determination and instruction execution
  uint32_t pc = 0x0;
  uint32_t raw_instr;
  uint32_t opcode;
  int op;
  int rs;
  int rt;
  int rd;
  int shamt;
  uint32_t funct;
  uint32_t imm;
  uint32_t jumpAddr;
  uint32_t newpc;
  int delaySlot = 0;

  // writes the given binary file into memory abstraction
  // establish stream and get size
  ifstream myFile (argv[1], ios::in | ios::binary);
  myFile.seekg(0, ios::end);
  size = myFile.tellg();
  // copy instructions from file to memory one word at a time
  for(int i = 0; i < size; i += 4, address += 0x4) {
    myFile.seekg(i, ios::beg); 
    myFile.read(buffer, 4);
    num = *(uint32_t *)buffer;
    num = ConvertWordToBigEndian(num);
    myMem->setMemValue(address, num, WORD_SIZE);
  }

  while(true){
    
    // retrieve next instruction from memory
    myMem->getMemValue(pc, raw_instr, WORD_SIZE);

    // if end of program, cleanup and dump memory and registers
    if (raw_instr == 0xfeedfeed){
      // copies register values into registerinfo r for register dump
      RegisterInfo regdump;
      regdump.at = r[1];
      for(int i = 2; i < 4; i++) {
        regdump.v[i - 2] = r[i];
      }
      for(int i = 4; i < 8; i++) {
        regdump.a[i - 4] = r[i];
      }
      for(int i = 8; i < 16; i++) {
        regdump.t[i - 8] = r[i];
      }
      for(int i = 16; i < 24; i++){
        regdump.s[i - 16] = r[i];
      }
      for(int i = 24; i < 26; i++){
        regdump.t[i - 16] = r[i];
      }
      for(int i = 26; i < 28; i++){
        regdump.k[i - 26] = r[i];
      }
      regdump.gp = r[28];
      regdump.sp = r[29];
      regdump.fp = r[30];
      regdump.ra = r[31];

      // dumps registers and memory state
      dumpRegisterState(regdump);
      dumpMemoryState(myMem);

      delete myMem;
      return 0;
    }

    // determine rs, rt, rd, shamt, funct, imm, addr
    rs = (raw_instr & 0x03e00000) >> 21;
    rt = (raw_instr & 0x001f0000) >> 16;
    rd = (raw_instr & 0x0000f800) >> 11;
    imm = raw_instr & 0x0000ffff;
    shamt = (raw_instr & 0x000007c0) >> 6;
    funct = raw_instr & 0x0000003f;
    jumpAddr = raw_instr & 0x03ffffff;

    // finds instruction type from opcode
    opcode = raw_instr >> 26;
    if (opcode == 0x0) {
      // depending on funct set the op string
    	if (funct == 0x20) {op = 1;} // add 
    	else if (funct == 0x21) {op = 2;} // addu 
    	else if (funct == 0x24) {op = 3;} // and 
    	else if (funct == 0x08) {op = 4;} // jr 
      else if (funct == 0x27) {op = 5;} // nor 
      else if (funct == 0x25) {op = 6;} // or 
      else if (funct == 0x2a) {op = 7;} // slt
      else if (funct == 0x2b) {op = 8;} // sltu
      else if (funct == 0x00) {op = 9;} // sll 
      else if (funct == 0x02) {op = 10;} // srl 
      else if (funct == 0x22) {op = 11;} // sub 
      else if (funct == 0x23) {op = 12;} // subu 
    } else if (opcode == 0x2){
      op = 13; // j 
    } else if (opcode == 0x3){
      op = 14; // jal 
    } else {
      // set op string depending on opcode
      if (opcode == 0x08) {op = 15;} // addi 
      else if (opcode == 0x09) {op = 16;} // addiu 
      else if (opcode == 0x0c) {op = 17;} // andi 
      else if (opcode == 0x04) {op = 18;} // beq 
      else if (opcode == 0x05) {op = 19;} // bne 
      else if (opcode == 0x24) {op = 20;} // lbu 
      else if (opcode == 0x25) {op = 21;} // lhu 
      else if (opcode == 0x0f) {op = 22;} // lui 
      else if (opcode == 0x23) {op = 23;} // lw 
      else if (opcode == 0x0d) {op = 24;} // ori 
      else if (opcode == 0x0a) {op = 25;} // slti 
      else if (opcode == 0x0b) {op = 26;} // sltiu 
      else if (opcode == 0x28) {op = 27;} // sb 
      else if (opcode == 0x29) {op = 28;} // sh 
      else if (opcode == 0x2b) {op = 29;} // sw 
      else if (opcode == 0x07) {op = 30;} // bgtz 
      else if (opcode == 0x06) {op = 31;} // blez 
    }

    // corner case: to handle empty delay slots
    if (raw_instr == 0x0){op == 0;}

    // execute operations according to type
    switch(op) {
      case 0:
        break;
      case 1: // add
        r[rd] = r[rs] + r[rt];
        break;
      case 2: // addu
        r[rd] = r[rs] + r[rt];
        break;
      case 3: // and
        r[rd] = r[rs] & r[rt];
      case 4: // jr
        newpc = r[rs];
        delaySlot = 1;
        break;
      case 5: // nor
        r[rd] = ~(r[rs]|r[rt]);
        break;
      case 6: // or
        r[rd] = r[rs]|r[rt];
        break;
      case 7: // slt
        imm = (int16_t) imm;
        r[rd] = (r[rs] < r[rt]) ? 0x1 : 0x0;
        break;
      case 8: // sltu
        imm = (uint16_t) imm;
        r[rd] = (r[rs] < r[rt]) ? 0x1 : 0x0;
        break;
      case 9: // sll
        r[rd] = r[rt] << shamt;
        break;
      case 10: // srl
        r[rd] = r[rt] >> shamt;
        break;
      case 11: // sub
        r[rd] = r[rs] - r[rt];
        break;
      case 12: // subu
        r[rd] = r[rs] - r[rt];
        break;
      case 13: // j
        newpc = (pc & 0xf0000000)|(jumpAddr << 2);
        delaySlot = 1;
        break;
      case 14: // jal
        newpc = (pc & 0xf0000000)|(jumpAddr << 2);
        r[31] = pc + 0x08;
        delaySlot = 1;
        break;
      case 15: // addi
        imm = (int16_t) imm;
        r[rt] = r[rs] + imm;
        break;
      case 16: // addiu
        imm = (int16_t) imm;
        r[rt] = r[rs] + imm;
        break;
      case 17: // andi
        imm = (uint16_t) imm;
        r[rd] = r[rs] & imm;
        break;
      case 18: // beq
        imm = (int16_t) imm;
        if (r[rs] == r[rt]){
          newpc = pc + 0x4 + (imm << 2);
          delaySlot = 1;
        }
        break;
      case 19: // bne
        imm = (int16_t) imm;
        if (r[rs] != r[rt]){
          newpc = pc + 0x4 + (imm << 2);
          delaySlot = 1;
        }
        break;
      case 20: // lbu
        imm = (int16_t) imm;
        myMem->getMemValue(r[rs] + imm, r[rt], BYTE_SIZE);
        break;
      case 21: // lhu
        imm = (int16_t) imm;
        myMem->getMemValue(r[rs] + imm, r[rt], HALF_SIZE);
        break;
      case 22: // lui
        r[rt] = imm << 16;
        break;
      case 23: // lw
        imm = (int16_t) imm;
        myMem->getMemValue(r[rs] + imm, r[rt], WORD_SIZE);
        break;
      case 24: // ori
        imm = (uint16_t) imm;
        r[rt] = r[rs]|imm;
        break;
      case 25: // slti
        imm = (int16_t) imm;
        r[rt] = (r[rs] < imm) ? 0x1 : 0x0;
        break;
      case 26: // sltiu
        imm = (uint16_t) imm;
        r[rt] = (r[rs] < imm) ? 0x1 : 0x0;
        break;
      case 27: // sb
        imm = (uint16_t) imm;
        myMem->setMemValue(r[rs] + imm, r[rt], BYTE_SIZE);
        break;
      case 28: // sh
        imm = (uint16_t) imm;
        myMem->setMemValue(r[rs] + imm, r[rt], HALF_SIZE);
        break;
      case 29: // sw
        imm = (uint16_t) imm;
        myMem->setMemValue(r[rs] + imm, r[rt], WORD_SIZE);
        break;
      case 30: // bgtz
        imm = (int16_t) imm;
        if (r[rs] > 0x0){
          newpc = pc + 0x4 + (imm << 2);
          delaySlot = 1;
        }
        break;
      case 31: // blez
        imm = (int16_t) imm;
        if (r[rs] <= 0x0){
          newpc = pc + 0x4 + (imm << 2);
          delaySlot = 1;
        }
        break;
      default:
        fprintf(stderr, "Illegal operation" + op);
        exit(127);
    }

    // handling delaySlots
    if (delaySlot == 0) {
      pc += 0x4;
    } else if (delaySlot == 1) {
      pc += 0x4;
      delaySlot = 2;
    } else if (delaySlot == 2) {
      pc = newpc;
      delaySlot = 0;
    }
  }
}

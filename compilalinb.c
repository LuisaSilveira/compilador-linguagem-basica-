/*
GABRIEL ROSAS 2210689 3WB
LUISA SILVEIRA 2210875 3WB
*/

#include "compilalinb.h"
#include <stdlib.h>

#define MAX_V 4
#define MAX_P 2
#define MAX_LINES 50

unsigned int V_MOV_ADD_SUB_R10D[] = {0xe2, 0xea, 0xf2, 0xfa};
unsigned int V_IMUL_R10D_MOV_CALLED_S[] = {0xd4, 0xd5, 0xd6, 0xd7};
unsigned int V_IF_VARP[] = {0xfc, 0xfd, 0xfe, 0xff};
unsigned int V_ALOCA_VAR_LOCAL[] = {0xca, 0xc0, 0xbc, 0xb8};
int line = 1;

static void error(const char *msg, int line);
void addVariavelLocal(unsigned char codigo[], int *i, int indx, char varx);
void operacao(int idx0, char var0, char var1, int idx1, char op, char var2,
              int idx2, unsigned char codigo[], int *i);
void desvio(unsigned char codigo[], int *i, char var0, int idx0, int jump[]);

funcp compilaLinB(FILE *f, unsigned char codigo[]) {
  int i = 0;
  int nJumps = 0;
  int jumps[MAX_LINES];
  long endereco_linhas[MAX_LINES];

  // Criar a pilha
  unsigned char init_code[] = {
      0x55,                   // pushq %rbp
      0x48, 0x89, 0xe5,       // movq %rsp, %rbp
      0x48, 0x83, 0xec, 0x30, // subq $48, %rsp
      0x4c, 0x89, 0x65, 0xf8, // mov %r12, -0x8(%rbp),
      0x4c, 0x89, 0x6d, 0xea, // mov %r13, -0x16(%rbp)
      0x4c, 0x89, 0x75, 0xdc, // mov %r14, -0x24(%rbp)
      0x4c, 0x89, 0x7d, 0xce  // mov %r15, -0x32(%rbp)
  };
  for (int k = 0; k < sizeof(init_code); k++) {
    codigo[i++] = init_code[k];
  }

  char c;
  while ((c = fgetc(f)) != EOF) {
    endereco_linhas[line] = i;
    switch (c) {
    case 'r': { // retorno
      char c0;
      if (fscanf(f, "et%c", &c0) != 1) {
        error("comando invalido", line);
      }
      // Destruindo a pilha
      unsigned char end_code[] = {
          0x44, 0x89, 0xe0,       // mov %r12d,%eax
          0x4c, 0x8b, 0x65, 0xf8, // mov -0x8(%rbp), %r12
          0x4c, 0x8b, 0x6d, 0xea, // mov -0x16(%rbp), %r13
          0x4c, 0x8b, 0x75, 0xdc, // mov -0x24(%rbp), %r14
          0x4c, 0x8b, 0x7d, 0xce, // mov -0x32(%rbp), %r15
          0xc9, 0xc3              // leave; ret
      };
      for (int k = 0; k < sizeof(end_code); k++) {
        codigo[i++] = end_code[k];
      }
      printf("ret\n");
      break;
    }
    case 'v':   // atribuicao
    case 'p': { // atribuicao
      int idx0, idx1, idx2;
      char var0 = c, var1, var2, op;
      if (fscanf(f, "%d <= %c%d %c %c%d", &idx0, &var1, &idx1, &op, &var2,
                 &idx2) != 6) {
        error("Comando Invalido", line);
      }
      operacao(idx0, var0, var1, idx1, op, var2, idx2, codigo, &i);
      printf("%c%d = %c%d %c %c%d\n", var0, idx0, var1, idx1, op, var2, idx2);
      break;
    }
    case 'i': { // desvio
      char var0;
      int idx0, num;
      if (fscanf(f, "f %c%d %d", &var0, &idx0, &num) != 3) {
        error("comando invalido", line);
      }

      desvio(codigo, &i, var0, idx0, jumps);
      jumps[nJumps++] = num;

      printf("if %c%d %d\n", var0, idx0, num);
      break;
    }
    default:
      error("comando desconhecido", line);
    }
    line++;
    if (line > MAX_LINES)
      error("Numero maximo de linhas ultrapassado", line);
    fscanf(f, " ");
  }
  int jmp = 0;
  for (int k = 0; k < i; k++) {
    if (codigo[k] == 0x0f && codigo[k + 1] == 0x85) {

      int destino = jumps[jmp++];
      int offset = endereco_linhas[destino] - (k + 6);
      *((int *)&codigo[k + 2]) = offset;
    }
  }

  return (funcp)codigo;
}

void operacao(int idx0, char var0, char var1, int idx1, char op, char var2,
              int idx2, unsigned char codigo[], int *i) {
  int j = *i;
  int isVar0 = (var0 == 'v');
  int isVar1 = (var1 == 'v');
  int isVar2 = (var2 == 'v');

  if (var1 == '$') {
    codigo[j++] = 0x41; codigo[j++] = 0xba; // mov %r10d, immediate
    *((int *)&codigo[j]) = idx1; j += 4; // $idx1
  } else {
    codigo[j++] = isVar1 ? 0x45 : 0x41; codigo[j++] = 0x89; // mov %r10d, register
    codigo[j++] = isVar1 ? V_MOV_ADD_SUB_R10D[idx1 - 1] : V_MOV_ADD_SUB_R10D[idx1 % 2 + 2]; // hex do registrador
  }

  switch (op) {
  case '+': {
    if (var2 == '$') {
      codigo[j++] = 0x41; codigo[j++] = 0x81; codigo[j++] = 0xc2; // add %r10d, immediate
      *((int *)&codigo[j]) = idx2; j += 4; // $idx2
      break;
    }
    codigo[j++] = isVar2 ? 0x45 : 0x41; codigo[j++] = 0x01; // add %r10d 
    codigo[j++] = isVar2 ? V_MOV_ADD_SUB_R10D[idx2 - 1] : V_MOV_ADD_SUB_R10D[idx2 % 2 + 2]; // hex do registrador
    break;
  }
  case '-': {
    if (var2 == '$') {
      codigo[j++] = 0x41; codigo[j++] = 0x81; codigo[j++] = 0xea; // sub %r10d
      *((int *)&codigo[j]) = idx2; j += 4; // $idx2
      break;
    } 
    codigo[j++] = isVar2 ? 0x45 : 0x41; codigo[j++] = 0x29; // sub %r10d
    codigo[j++] = isVar2 ? V_MOV_ADD_SUB_R10D[idx2 - 1] : V_MOV_ADD_SUB_R10D[idx2 % 2 + 2]; // hex do registrador
    break;
  }
  case '*': {
    if (var2 == '$') {
      codigo[j++] = 0x45; codigo[j++] = 0x69; codigo[j++] = 0xd2; // imul %r10d
      *((int *)&codigo[j]) = idx2; j += 4; // $idx2
      break;
    }
    codigo[j++] = isVar2 ? 0x45 : 0x44; codigo[j++] = 0x0f; codigo[j++] = 0xaf; // imul %r10d
    codigo[j++] = isVar2 ? V_IMUL_R10D_MOV_CALLED_S[idx2 - 1] : V_IMUL_R10D_MOV_CALLED_S[idx0 % 2 + 2]; // hex do registrador
    break;
  }
  }
  codigo[j++] = isVar0 ? 0x45 : 0x44; codigo[j++] = 0x89; // mov $r10d
  codigo[j++] = isVar0 ? V_IMUL_R10D_MOV_CALLED_S[idx0 - 1] : V_IMUL_R10D_MOV_CALLED_S[idx0 % 2 + 2]; // hex do registrador
  if (isVar0) addVariavelLocal(codigo, &j, idx0, var0);
  *i = j;
}

void desvio(unsigned char codigo[], int *i, char var0, int idx0, int jump[]) {
  int j = *i;
  int isVar = (var0 == 'v');

  // cmp
  if (isVar) codigo[j++] = 0x41;
  codigo[j++] = 0x83; codigo[j++] = isVar ? V_IF_VARP[idx0 - 1] : V_IF_VARP[idx0 % 2 + 2]; 
  codigo[j++] = 0x00;

  // jne
  codigo[j++] = 0x0f; codigo[j++] = 0x85;
  for (int k = 0; k < 4; k++)
    codigo[j++] = 0x00;
  *i = j;
}

static void error(const char *msg, int line) {
  fprintf(stderr, "erro %s na linha %d\n", msg, line);
  exit(EXIT_FAILURE);
}

void addVariavelLocal(unsigned char codigo[], int *i, int indx, char varx) {
  int j = *i;
  codigo[j++] = 0x44; codigo[j++] = 0x89;
  codigo[j++] = 0x65 + 8 * (indx - 1);
  codigo[j++] = V_ALOCA_VAR_LOCAL[indx - 1];
  *i = j;
}
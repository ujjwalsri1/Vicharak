#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 50

// Token types
typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_PAREN_OPEN,
    TOKEN_PAREN_CLOSE,
    TOKEN_ASSIGN,
    TOKEN_IF,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char text[MAX_TOKEN_LENGTH];
} Token;

// AST Node types
typedef enum {
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,
    AST_IF,
    AST_NUMBER
} ASTNodeType;

// AST Node structure
typedef struct ASTNode {
    ASTNodeType type;
    char value[MAX_TOKEN_LENGTH];
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

// Global variables
Token tokens[MAX_TOKENS];
int tokenCount = 0;

// Function prototypes
void tokenizeFile(const char* filename);
ASTNode* parseExpression(int* index);
ASTNode* parseTerm(int* index);
ASTNode* parseFactor(int* index);
ASTNode* parseAssignment(int* index);
ASTNode* parseIfStatement(int* index);
void generateAssembly(ASTNode* node);
void freeAST(ASTNode* node);

// Function to create a new AST node
ASTNode* createNode(ASTNodeType type, const char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    strcpy(node->value, value);
    node->left = node->right = NULL;
    return node;
}

// Function to tokenize the input file
void tokenizeFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char ch;
    char token[MAX_TOKEN_LENGTH];
    int tokenIndex = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (isspace(ch)) {
            if (tokenIndex > 0) {
                token[tokenIndex] = '\0';
                tokens[tokenCount].type = TOKEN_UNKNOWN;
                strcpy(tokens[tokenCount++].text, token);
                tokenIndex = 0;
            }
            continue;
        }

        if (isalpha(ch)) {
            token[tokenIndex++] = ch;
            while (isalnum(ch = fgetc(file))) {
                token[tokenIndex++] = ch;
            }
            ungetc(ch, file);
            token[tokenIndex] = '\0';
            tokens[tokenCount].type = TOKEN_IDENTIFIER;
            strcpy(tokens[tokenCount++].text, token);
            tokenIndex = 0;
        } else if (isdigit(ch)) {
            token[tokenIndex++] = ch;
            while (isdigit(ch = fgetc(file))) {
                token[tokenIndex++] = ch;
            }
            ungetc(ch, file);
            token[tokenIndex] = '\0';
            tokens[tokenCount].type = TOKEN_NUMBER;
            strcpy(tokens[tokenCount++].text, token);
            tokenIndex = 0;
        } else if (ch == '=') {
            tokens[tokenCount].type = TOKEN_ASSIGN;
            tokens[tokenCount].text[0] = ch;
            tokens[tokenCount++].text[1] = '\0';
        } else if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
            tokens[tokenCount].type = TOKEN_OPERATOR;
            tokens[tokenCount].text[0] = ch;
            tokens[tokenCount++].text[1] = '\0';
        } else if (ch == '(') {
            tokens[tokenCount].type = TOKEN_PAREN_OPEN;
            tokens[tokenCount].text[0] = ch;
            tokens[tokenCount++].text[1] = '\0';
        } else if (ch == ')') {
            tokens[tokenCount].type = TOKEN_PAREN_CLOSE;
            tokens[tokenCount].text[0] = ch;
            tokens[tokenCount++].text[1] = '\0';
        } else if (ch == '{' || ch == '}') {
            // Ignore braces for now
        } else {
            tokens[tokenCount].type = TOKEN_UNKNOWN;
            tokens[tokenCount].text[0] = ch;
            tokens[tokenCount++].text[1] = '\0';
        }
    }

    fclose(file);
}

// Function to parse an expression
ASTNode* parseExpression(int* index) {
    ASTNode* left = parseTerm(index);
    while (tokens[*index].type == TOKEN_OPERATOR &&
           (tokens[*index].text[0] == '+' || tokens[*index].text[0] == '-')) {
        char op = tokens[*index].text[0];
        (*index)++;
        ASTNode* right = parseTerm(index);
        ASTNode* newNode = createNode(op == '+' ? AST_ADD : AST_SUB, "");
        newNode->left = left;
        newNode->right = right;
        left = newNode;
    }
    return left;
}

// Function to parse a term
ASTNode* parseTerm(int* index) {
    ASTNode* left = parseFactor(index);
    while (tokens[*index].type == TOKEN_OPERATOR &&
           (tokens[*index].text[0] == '*' || tokens[*index].text[0] == '/')) {
        char op = tokens[*index].text[0];
        (*index)++;
        ASTNode* right = parseFactor(index);
        ASTNode* newNode = createNode(op == '*' ? AST_MUL : AST_DIV, "");
        newNode->left = left;
        newNode->right = right;
        left = newNode;
    }
    return left;
}

// Function to parse a factor
ASTNode* parseFactor(int* index) {
    if (tokens[*index].type == TOKEN_NUMBER) {
        return createNode(AST_NUMBER, tokens[(*index)++].text);
    } else if (tokens[*index].type == TOKEN_IDENTIFIER) {
        return createNode(AST_VAR_DECL, tokens[(*index)++].text);
    } else if (tokens[*index].type == TOKEN_PAREN_OPEN) {
        (*index)++;
        ASTNode* node = parseExpression(index);
        if (tokens[*index].type != TOKEN_PAREN_CLOSE) {
            fprintf(stderr, "Syntax error: expected ')'\n");
            exit(1);
        }
        (*index)++;
        return node;
    } else {
        fprintf(stderr, "Syntax error: unexpected token\n");
        exit(1);
    }
}

// Function to parse an assignment
ASTNode* parseAssignment(int* index) {
    ASTNode* node = createNode(AST_ASSIGN, tokens[*index].text);
    (*index)++;
    node->left = createNode(AST_VAR_DECL, tokens[*index].text);
    (*index)++;

    if (tokens[*index].type == TOKEN_ASSIGN) {
        (*index)++;
        node->right = parseExpression(index);
    }
    return node;
}

// Function to parse an if statement
ASTNode* parseIfStatement(int* index) {
    (*index)++;  // Skip 'if'
    if (tokens[*index].type == TOKEN_PAREN_OPEN) {
        (*index)++;
        ASTNode* condition = parseExpression(index);
        if (tokens[*index].type != TOKEN_PAREN_CLOSE) {
            fprintf(stderr, "Syntax error: expected ')'\n");
            exit(1);
        }
        (*index)++;
        ASTNode* ifNode = createNode(AST_IF, "");
        ifNode->left = condition;
        ifNode->right = parseAssignment(index);
        return ifNode;
    }
    return NULL;
}

// Function to generate assembly code
void generateAssembly(ASTNode* node) {
    if (node == NULL) return;

    if (node->type == AST_ASSIGN) {
        printf("MOV %s, %s\n", node->left->value, node->right->value);
    } else if (node->type == AST_ADD) {
        printf("ADD %s, %s\n", node->left->value, node->right->value);
    } else if (node->type == AST_SUB) {
        printf("SUB %s, %s\n", node->left->value, node->right->value);
    } else if (node->type == AST_MUL) {
        printf("MUL %s, %s\n", node->left->value, node->right->value);
    } else if (node->type == AST_DIV) {
        printf("DIV %s, %s\n", node->left->value, node->right->value);
    } else if (node->type == AST_VAR_DECL) {
        printf("DECLARE %s\n", node->value);
    } else if (node->type == AST_NUMBER) {
        printf("PUSH %s\n", node->value);
    } else if (node->type == AST_IF) {
        printf("IF %s == 0 GOTO LABEL\n", node->left->value);
        generateAssembly(node->right);
    }
}

// Function to free the AST
void freeAST(ASTNode* node) {
    if (node == NULL) return;
    freeAST(node->left);
    freeAST(node->right);
    free(node);
}

// Main function
int main() {
    // Tokenize the input file
    tokenizeFile("input.txt");

    // Parse and generate assembly code
    int index = 0;
    while (index < tokenCount) {
        if (strcmp(tokens[index].text, "if") == 0) {
            ASTNode* ast = parseIfStatement(&index);
            generateAssembly(ast);
            freeAST(ast);
        } else {
            ASTNode* ast = parseAssignment(&index);
            generateAssembly(ast);
            freeAST(ast);
        }
    }

    return 0;
}
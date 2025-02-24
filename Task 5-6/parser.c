#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 50

typedef enum { 
    AST_VAR_DECL, AST_ASSIGN, AST_ADD, AST_SUB, AST_MUL, AST_DIV, AST_IF, AST_NUMBER 
} ASTNodeType;

typedef struct {
    char text[MAX_TOKEN_LENGTH];
} Token;

typedef struct ASTNode {
    ASTNodeType type;
    char value[50];
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

Token tokens[MAX_TOKENS];
int tokenCount = 0;

// Function prototypes
ASTNode* parseExpression(Token tokens[], int* index);
ASTNode* parseTerm(Token tokens[], int* index);
ASTNode* parseFactor(Token tokens[], int* index);
ASTNode* parseAssignment(Token tokens[], int* index);
ASTNode* parseIfStatement(Token tokens[], int* index);

// Function to create a new AST node
ASTNode* createNode(ASTNodeType type, const char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    strcpy(node->value, value);
    node->left = node->right = NULL;
    return node;
}

ASTNode* parseAssignment(Token tokens[], int* index) {
    ASTNode* node = createNode(AST_ASSIGN, tokens[*index].text);  // Assignment node
    (*index)++;
    node->left = createNode(AST_VAR_DECL, tokens[*index].text);   // Variable declaration (left side)
    (*index)++;

    if (strcmp(tokens[*index].text, "=") == 0) {  // Check for '='
        (*index)++;
        node->right = parseExpression(tokens, index);  // Parse the right-hand expression
    }
    return node;
}

ASTNode* parseExpression(Token tokens[], int* index) {
    ASTNode* left = parseTerm(tokens, index);  // Parse the first term

    while (tokens[*index].text[0] == '+' || tokens[*index].text[0] == '-') {
        char op = tokens[*index].text[0];  // Operator ('+' or '-')
        (*index)++;
        ASTNode* right = parseTerm(tokens, index);  // Parse the next term
        
        ASTNode* newNode = createNode(op == '+' ? AST_ADD : AST_SUB, "");  // Create ADD/SUB node
        newNode->left = left;
        newNode->right = right;
        left = newNode;  // Update left to the new node
    }

    return left;  // Return the full expression tree
}

// Function to parse a term (multiplication or division)
ASTNode* parseTerm(Token tokens[], int* index) {
    ASTNode* left = parseFactor(tokens, index);  // Parse the first factor

    // Handle multiplication or division
    while (tokens[*index].text[0] == '*' || tokens[*index].text[0] == '/') {
        char op = tokens[*index].text[0];  // Operator ('*' or '/')
        (*index)++;
        ASTNode* right = parseFactor(tokens, index);  // Parse the next factor
        
        ASTNode* newNode = createNode(op == '*' ? AST_MUL : AST_DIV, "");  // Create MUL/DIV node
        newNode->left = left;
        newNode->right = right;
        left = newNode;  // Update left to the new node
    }

    return left;  // Return the full term tree
}

// Function to parse a factor (number or variable)
ASTNode* parseFactor(Token tokens[], int* index) {
    if (isdigit(tokens[*index].text[0])) {
        return createNode(AST_NUMBER, tokens[(*index)++].text);  // Number
    } else {
        return createNode(AST_VAR_DECL, tokens[(*index)++].text);  // Variable
    }
}

// Function to parse an if statement
ASTNode* parseIfStatement(Token tokens[], int* index) {
    (*index)++;  // Skip 'if'
    if (strcmp(tokens[*index].text, "(") == 0) {
        (*index)++;
        ASTNode* condition = parseExpression(tokens, index);  // Parse the condition
        
        if (strcmp(tokens[*index].text, ")") == 0) {
            (*index)++;
            if (strcmp(tokens[*index].text, "{") == 0) {
                (*index)++;
                ASTNode* ifNode = createNode(AST_IF, "");  // Create an 'if' node
                ifNode->left = condition;  // Store the condition in left child
                
                // Parse statements inside the if block
                ASTNode* statement = parseAssignment(tokens, index);
                ifNode->right = statement;
                
                if (strcmp(tokens[*index].text, "}") == 0) {
                    (*index)++;
                }
                return ifNode;
            }
        }
    }
    return NULL;
}

// Function to generate assembly code
void generateAssembly(ASTNode* node) {
    if (node == NULL) return;

    if (node->type == AST_ASSIGN) {
        printf("MOV %s, %s\n", node->left->value, node->right->value);  // Assign right side to left side
    } else if (node->type == AST_ADD) {
        printf("ADD %s, %s\n", node->left->value, node->right->value);  // Add two operands
    } else if (node->type == AST_SUB) {
        printf("SUB %s, %s\n", node->left->value, node->right->value);  // Subtract two operands
    } else if (node->type == AST_MUL) {
        printf("MUL %s, %s\n", node->left->value, node->right->value);  // Multiply two operands
    } else if (node->type == AST_DIV) {
        printf("DIV %s, %s\n", node->left->value, node->right->value);  // Divide two operands
    } else if (node->type == AST_VAR_DECL) {
        printf("DECLARE %s\n", node->value);  // Declare variable
    } else if (node->type == AST_NUMBER) {
        printf("PUSH %s\n", node->value);  // Push number onto stack
    } else if (node->type == AST_IF) {
        printf("IF %s == 0 GOTO LABEL\n", node->left->value);  // If condition (dummy code, needs label logic)
        generateAssembly(node->right);  // Generate code for if block
    }
}

// Function to read the file and extract tokens
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
        if (isspace(ch) || ch == '=' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(' || ch == ')' || ch == '{' || ch == '}') {
            if (tokenIndex > 0) {
                token[tokenIndex] = '\0';
                strcpy(tokens[tokenCount++].text, token);
                tokenIndex = 0;
            }
            if (ch == '=' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '(' || ch == ')' || ch == '{' || ch == '}') {
                token[0] = ch;
                token[1] = '\0';
                strcpy(tokens[tokenCount++].text, token);
            }
        } else if (isalnum(ch)) {
            token[tokenIndex++] = ch;
        }
    }
    if (tokenIndex > 0) {
        token[tokenIndex] = '\0';
        strcpy(tokens[tokenCount++].text, token);
    }

    fclose(file);
}

// Main function to generate assembly code from the tokens
int main() {
    tokenizeFile("input.txt");  // Read and tokenize the content of input.txt

    // Print tokens for debugging
    for (int i = 0; i < tokenCount; i++) {
        printf("Token %d: %s\n", i + 1, tokens[i].text);
    }

    int tokenIndex = 0;
    ASTNode* ast = NULL;
    
    // Parse the full program
    while (tokenIndex < tokenCount) {
        if (strcmp(tokens[tokenIndex].text, "if") == 0) {
            ast = parseIfStatement(tokens, &tokenIndex);  // Parse if statement
        } else {
            ast = parseAssignment(tokens, &tokenIndex);  // Parse assignment
        }
        
        generateAssembly(ast);  // Generate assembly code for each parsed statement
    }
    
    return 0;  // Correctly close the main function
}
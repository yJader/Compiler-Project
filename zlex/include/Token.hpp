#ifndef TOKEN_H
#define TOKEN_H
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_set>
#include <struct.hpp>
typedef enum
{
    T_Int = 256,
    T_Float,
    T_Double,
    T_Long,
    T_If,
    T_Else,
    T_While,
    T_For,
    T_Do,
    T_Break,
    T_Continue,
    T_Return,
    T_Void,
    T_Main,
    T_Le,
    T_Ge,
    T_Equal,
    T_Noequal,
    T_Increment,
    T_Decrement,
    T_Add_Assignment,
    T_Sub_Assignment,
    T_Mul_Assignment,
    T_Divid_Assignment,
    T_Mode_Assignment,
    T_And,
    T_Or
} TokenType;

// 符号表
namespace SymbolTable
{
    void addToSymbolTable(std::string type, std::string name);
    void printSymbolTable(std::ofstream &symbolOut);
    extern std::unordered_set<std::pair<std::string, std::string>, pair_hash> symbolTable;
}

class Token
{
public:
    Token(std::string character, std::ofstream &tokenOut, bool isDigit = false);
    static std::map<std::string, int> operatorMap;
    void print_token();

private:
    std::string character;
    std::ofstream &tokenOut;
    bool isDigit;
};

#endif
#pragma once

/** 
 * Def 语法分析树
 */


#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <iostream>

#include "../global.h"

#include "./type.h"
#include "./error.h"

#include "../compile/gen.h"
#include "../parse/tokenizer.h"
#include "../util/str.h"

#include "llvm/IR/IRBuilder.h"

namespace def {
namespace core {
    
using namespace std;
using namespace def::util;
using namespace def::parse;
using namespace def::compile;


/**
 * Def 抽象解析树 
 */
struct AST
{
    virtual llvm::Value* codegen(Gen &) { return nullptr;  };
    virtual void print(string pre="", string ind="") {};
    virtual def::core::Type* getType() {
        FATAL("cannot call getType() , this AST is not a <value> !")
    };
    // 是否为 value 值（除了函数、类型的声明、定义等）
    virtual bool isValue() { return false; }; 
};


class ASTTypeDefine;
class ASTFunctionDefine;



#define AST_HEAD(N) \
struct AST##N : AST \
{ \
    virtual void print(string, string); \


#define AST_CODE_HEAD(N) \
AST_HEAD(N) \
    virtual llvm::Value* codegen(Gen &); \
    virtual def::core::Type* getType(); \
    virtual bool isValue() { return true; }; \


// 块
AST_CODE_HEAD(Group)
    vector<AST*> childs; // 列表
    void add(AST*a); // 添加子句
};


// 常量
AST_CODE_HEAD(Constant)
    Type* type; // 常量类型
    string value; // 常量字面值
    ASTConstant(Type*t, const string&v)
        : type(t)
        , value(v)
    {}
};


// 引用值
AST_CODE_HEAD(Quote)
    Type* type;   // 引用类型
    AST* value;   // 引用的值
    ASTQuote(AST* v, Type* t)
        : value(v)
        , type(t)
    {}
};


// 从引用载入值
AST_CODE_HEAD(Load)
    Type* type;   // 引用类型
    AST* value;   // 引用的值
    ASTLoad(AST* v, Type* t)
        : value(v)
        , type(t)
    {}
};



// 函数返回值
AST_CODE_HEAD(Ret)
    AST* value;
    ASTRet(AST*v)
        : value(v)
    {}
};


// if 条件分支
AST_CODE_HEAD(If)
    bool canphi=false; // IF分支的类型一致，可建立 PHI 节点
    AST* cond;
    AST* pthen=nullptr;
    AST* pelse=nullptr;
    ASTIf(AST*c)
        : cond(c)
    {}
};


// while 循环
AST_CODE_HEAD(While)
    AST* cond;
    AST* body=nullptr;
    ASTWhile(AST*c)
        : cond(c)
    {}
};


// 类型定义
AST_HEAD(TypeDefine)
    TypeStruct* type;
    vector<ASTFunctionDefine*> members; // 成员函数
    // bool checkMember(ASTFuntionDefine* fdef);
    // void addMember(ASTFuntionDefine* fdef);
};


// 外部成员函数定义
AST_HEAD(ExternalMemberFunctionDefine)
    TypeStruct* type;
    AST* defs;
    ASTExternalMemberFunctionDefine(TypeStruct*t=nullptr,AST*c=nullptr)
        : type(t)
        , defs(c)
    {}
};



// 类型构造
AST_CODE_HEAD(TypeConstruct)
    TypeStruct* type;
    vector<AST*> childs; //
    bool bare = false; // 空构造
    ASTTypeConstruct(TypeStruct*t=nullptr, bool b=false)
        : type(t)
        , bare(b)
    {}
    void add(AST*);
};


// 变量
AST_CODE_HEAD(Variable)
    string name; // 名称
    Type* type;  // 类型
    ASTVariable(const string&n, Type*t)
        : name(n)
        , type(t)
    {}
};


// 变量定义
AST_CODE_HEAD(VariableDefine)
    string name;
    AST* value; // 
    ASTVariableDefine(const string &n = "", AST*v = nullptr)
        : name(n)
        , value(v)
    {}
};


// 变量赋值
AST_CODE_HEAD(VariableAssign)
    string name;
    AST* value; // 
    ASTVariableAssign(const string &n = "", AST*v = nullptr)
        : name(n)
        , value(v)
    {}
};


// 函数声明
AST_HEAD(FuntionDeclare)
    TypeFunction* ftype; // 函数类型
    ASTFuntionDeclare(TypeFunction*ft)
        : ftype(ft)
    {}
};


// 函数定义
AST_HEAD(FunctionDefine)
    TypeFunction* ftype; // 函数类型
    ASTGroup* body; // 函数体
    ASTFunctionDefine* wrap = nullptr; // 外层函数
    ASTTypeDefine*  belong = nullptr; // 所属类
    bool is_static_member  = true;    // 是否为静态成员函数
    bool is_construct  = false;    // 是否为构造函数
    set<string> cptmbr;        // 捕获使用的类成员名称
    map<string, Type*> cptvar;  // 捕获外层作用域的变量
    ASTFunctionDefine(TypeFunction*ft, ASTGroup *bd=nullptr)
        : ftype(ft)
        , body(bd)
    {}
    string getWrapPrefix(); // 获取外层函数前缀
    string getIdentify();   // 获取唯一的函数名称
};

// 函数调用
AST_CODE_HEAD(FunctionCall)
    ASTFunctionDefine* fndef; // 函数定义
    vector<AST*> params; // 实参值表
    ASTFunctionCall(ASTFunctionDefine*fd=nullptr)
        : fndef(fd)
    {}
    void addparam(AST*);// 添加实参
};

// 成员函数调用
AST_CODE_HEAD(MemberFunctionCall)
    ASTFunctionCall* call; // 函数定义
    AST* value; // 类实例
    ASTMemberFunctionCall(AST*v=nullptr, ASTFunctionCall*c=nullptr)
        : call(c)
        , value(v)
    {}
};

// 成员访问
AST_CODE_HEAD(MemberVisit)
    size_t index; // 子元素索引
    AST* instance; // 类实例
    ASTMemberVisit(AST*v=nullptr, size_t i=0)
        : index(i)
        , instance(v)
    {}
};

// 成员赋值
AST_CODE_HEAD(MemberAssign)
    size_t index; // 子元素索引
    AST* instance; // 类实例
    AST* value; // 赋值
    ASTMemberAssign(AST*m=nullptr, size_t i=0, AST*c=nullptr)
        : index(i)
        , instance(m)
        , value(c)
    {}
};



// 模板函数定义
AST_HEAD(TemplateFuntionDefine)
    string name;
    vector<string> params;
    list<Tokenizer::Word> bodywords; // 函数体单词表
    ASTTemplateFuntionDefine()
    {}
    void addword(string);
};


// let 符号绑定
AST_HEAD(Let)
    vector<string> head;
    vector<string> body;
    ASTLet()
    {}
};






#undef AST_HEAD



}
}


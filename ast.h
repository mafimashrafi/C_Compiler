#ifndef AST_H
#define AST_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>

using namespace std;

class ASTNode {
public:
    virtual ~ASTNode() {}
    virtual string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp, int& temp_count, int& label_count, bool inside_label = false) const = 0;
};

// Expression node types

class ExprNode :  public ASTNode {
protected:
    string node_type;
public:
    ExprNode(string type) : node_type(type) {}
    virtual string get_type() const { return node_type; }
};

// Variable node (for ID references)

class VarNode :  public ExprNode {
private:
    string name;
    ExprNode* index;
    bool owns_index;

public:
    VarNode(string name, string type, ExprNode* idx = nullptr, bool own_idx = true)
        : ExprNode(type), name(name), index(idx), owns_index(own_idx) {}
    
    ~VarNode() { 
        if(index && owns_index) delete index; 
    }
    
    bool has_index() const { return index != nullptr; }
    
    string generate_index_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                              int& temp_count, int& label_count, bool inside_label = false) const {
        if (index) {
            return index->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        }
        return "";
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        if (has_index()) {
            string index_temp = generate_index_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
            string result_temp = "t" + to_string(temp_count++);
            outcode << result_temp << " = " << name << "[" << index_temp << "]" << endl;
            return result_temp;
        }
        
        // Check if this variable already has a cached temp
        if (symbol_to_temp. find(name) != symbol_to_temp.end()) {
            // Only increment temp_count if inside a label block (control flow body)
            if (inside_label) {
                temp_count++;
            }
            // Don't emit anything, just return the cached temp
            return symbol_to_temp[name];
        }
        
        // First time seeing this variable - generate load and cache it
        string temp = "t" + to_string(temp_count++);
        outcode << temp << " = " << name << endl;
        symbol_to_temp[name] = temp;
        return temp;
    }
    
    string get_name() const { return name; }
};

// Constant node

class ConstNode : public ExprNode {
private: 
    string value;

public:
    ConstNode(string val, string type) : ExprNode(type), value(val) {}
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string temp = "t" + to_string(temp_count++);
        outcode << temp << " = " << value << endl;
        return temp;
    }
};

// Binary operation node

class BinaryOpNode : public ExprNode {
private:
    string op;
    ExprNode* left;
    ExprNode* right;
    bool owns_children;

public: 
    BinaryOpNode(string op, ExprNode* left, ExprNode* right, string result_type, bool owns = true)
        : ExprNode(result_type), op(op), left(left), right(right), owns_children(owns) {}
    
    ~BinaryOpNode() {
        if (owns_children) {
            delete left;
            delete right;
        }
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string left_temp = left->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        string right_temp = right->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        
        string result_temp = "t" + to_string(temp_count++);
        outcode << result_temp << " = " << left_temp << " " << op << " " << right_temp << endl;
        return result_temp;
    }
};

// Unary operation node

class UnaryOpNode : public ExprNode {
private:
    string op;
    ExprNode* expr;

public:
    UnaryOpNode(string op, ExprNode* expr, string result_type)
        : ExprNode(result_type), op(op), expr(expr) {}
    
    ~UnaryOpNode() { delete expr; }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string expr_temp = expr->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        string result_temp = "t" + to_string(temp_count++);
        outcode << result_temp << " = " << op << expr_temp << endl;
        return result_temp;
    }
};

// Assignment node

class AssignNode : public ExprNode {
private: 
    VarNode* lhs;
    ExprNode* rhs;
    bool owns_lhs;

public: 
    AssignNode(VarNode* lhs, ExprNode* rhs, string result_type, bool own_lhs = true)
        : ExprNode(result_type), lhs(lhs), rhs(rhs), owns_lhs(own_lhs) {}
    
    ~AssignNode() {
        if (owns_lhs) delete lhs;
        delete rhs;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string rhs_temp = rhs->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        
        if (lhs->has_index()) {
            string index_temp = lhs->generate_index_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
            outcode << lhs->get_name() << "[" << index_temp << "] = " << rhs_temp << endl;
        } else {
            outcode << lhs->get_name() << " = " << rhs_temp << endl;
        }
        
        return rhs_temp;
    }
};

// Statement node types

class StmtNode : public ASTNode {
public: 
    virtual string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                                int& temp_count, int& label_count, bool inside_label = false) const = 0;
};

// Expression statement node

class ExprStmtNode : public StmtNode {
private:
    ExprNode* expr;
    bool owns_expr;

public:
    ExprStmtNode(ExprNode* e, bool owns = true) : expr(e), owns_expr(owns) {}
    ~ExprStmtNode() { 
        if(expr && owns_expr) delete expr; 
    }
    
    ExprNode* get_expr() const { return expr; }
    
    void set_owns_expr(bool owns) { owns_expr = owns; }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        if (expr) {
            return expr->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        }
        return "";
    }
};

// Block (compound statement) node

class BlockNode : public StmtNode {
private: 
    vector<StmtNode*> statements;

public:
    ~BlockNode() {
        for (auto stmt : statements) {
            delete stmt;
        }
    }
    
    void add_statement(StmtNode* stmt) {
        if (stmt) statements.push_back(stmt);
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string last_result = "";
        for (auto stmt : statements) {
            last_result = stmt->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        }
        return last_result;
    }
};

// If statement node

class IfNode : public StmtNode {
private: 
    ExprNode* condition;
    StmtNode* then_block;
    StmtNode* else_block;

public:
    IfNode(ExprNode* cond, StmtNode* then_stmt, StmtNode* else_stmt = nullptr)
        : condition(cond), then_block(then_stmt), else_block(else_stmt) {}
    
    ~IfNode() {
        delete condition;
        delete then_block;
        if (else_block) delete else_block;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        // Condition is evaluated BEFORE entering label block
        string cond_temp = condition->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        
        string true_label = "L" + to_string(label_count++);
        string false_label = "L" + to_string(label_count++);
        string end_label = "L" + to_string(label_count++);
        
        outcode << "if " << cond_temp << " goto " << true_label << endl;
        outcode << "goto " << false_label << endl;
        outcode << true_label << ":" << endl;
        
        // Inside the label block - pass true for inside_label
        then_block->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
        
        if (else_block) {
            outcode << "goto " << end_label << endl;
            outcode << false_label << ":" << endl;
            // Inside the else label block - pass true for inside_label
            else_block->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
            outcode << end_label << ":" << endl;
        } else {
            outcode << "goto " << end_label << endl;
            outcode << false_label << ":" << endl;
            outcode << end_label << ":" << endl;
        }
        
        return "";
    }
};

// While statement node

class WhileNode : public StmtNode {
private:
    ExprNode* condition;
    StmtNode* body;

public:
    WhileNode(ExprNode* cond, StmtNode* body_stmt)
        : condition(cond), body(body_stmt) {}
    
    ~WhileNode() {
        delete condition;
        delete body;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        string start_label = "L" + to_string(label_count++);
        string true_label = "L" + to_string(label_count++);
        string end_label = "L" + to_string(label_count++);
        
        outcode << start_label << ":" << endl;
        // Condition is inside the loop structure but before the body label
        string cond_temp = condition->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
        outcode << "if " << cond_temp << " goto " << true_label << endl;
        outcode << "goto " << end_label << endl;
        outcode << true_label << ":" << endl;
        
        // Inside the label block - pass true for inside_label
        body->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
        
        outcode << "goto " << start_label << endl;
        outcode << end_label << ":" << endl;
        
        return "";
    }
};

// For statement node

class ForNode : public StmtNode {
private: 
    ExprNode* init;
    ExprNode* condition;
    ExprNode* update;
    StmtNode* body;
    bool owns_exprs;

public: 
    ForNode(ExprNode* init_expr, ExprNode* cond_expr, ExprNode* update_expr, StmtNode* body_stmt, bool owns = false)
        : init(init_expr), condition(cond_expr), update(update_expr), body(body_stmt), owns_exprs(owns) {}
    
    ~ForNode() {
        if (owns_exprs) {
            if (init) delete init;
            if (condition) delete condition;
            if (update) delete update;
        }
        delete body;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        // Init is before the loop labels
        if (init) {
            init->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
        }
        
        string start_label = "L" + to_string(label_count++);
        string true_label = "L" + to_string(label_count++);
        string end_label = "L" + to_string(label_count++);
        
        outcode << start_label << ":" << endl;
        
        if (condition) {
            // Condition is inside the loop structure
            string cond_temp = condition->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
            outcode << "if " << cond_temp << " goto " << true_label << endl;
            outcode << "goto " << end_label << endl;
            outcode << true_label << ":" << endl;
        } else {
            outcode << true_label << ":" << endl;
        }
        
        // Inside the label block - pass true for inside_label
        body->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
        
        if (update) {
            // Update is inside the loop
            update->generate_code(outcode, symbol_to_temp, temp_count, label_count, true);
        }
        
        outcode << "goto " << start_label << endl;
        outcode << end_label << ":" << endl;
        
        return "";
    }
};

// Return statement node

class ReturnNode :  public StmtNode {
private: 
    ExprNode* expr;

public: 
    ReturnNode(ExprNode* e) : expr(e) {}
    ~ReturnNode() { if (expr) delete expr; }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        if (expr) {
            string result = expr->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
            outcode << "return " << result << endl;
            return result;
        } else {
            outcode << "return" << endl;
            return "";
        }
    }
};

// Declaration node

class DeclNode : public StmtNode {
private:
    string type;
    vector<pair<string, int>> vars;

public:
    DeclNode(string t) : type(t) {}
    
    void add_var(string name, int array_size = 0) {
        vars.push_back(make_pair(name, array_size));
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        for (auto& var :  vars) {
            if (var.second > 0) {
                outcode << "// Declaration: " << type << " " << var.first << "[" << var.second << "]" << endl;
            } else {
                outcode << "// Declaration: " << type << " " << var. first << endl;
            }
        }
        return "";
    }
    
    string get_type() const { return type; }
    const vector<pair<string, int>>& get_vars() const { return vars; }
};

// Function declaration node

class FuncDeclNode : public ASTNode {
private:
    string return_type;
    string name;
    vector<pair<string, string>> params;
    BlockNode* body;

public:
    FuncDeclNode(string ret_type, string n) : return_type(ret_type), name(n), body(nullptr) {}
    ~FuncDeclNode() { if (body) delete body; }
    
    void add_param(string type, string name) {
        params. push_back(make_pair(type, name));
    }
    
    void set_body(BlockNode* b) {
        body = b;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        // Clear symbol_to_temp for new function scope
        map<string, string> local_symbol_map;
        
        outcode << endl << "// Function: " << return_type << " " << name << "(";
        for (size_t i = 0; i < params.size(); i++) {
            outcode << params[i].first << " " << params[i].second;
            if (i < params.size() - 1) outcode << ", ";
        }
        outcode << ")" << endl;
        
        // Load parameters and store their temps in local map
        for (auto& param : params) {
            string temp = "t" + to_string(temp_count++);
            outcode << temp << " = " << param.second << endl;
            local_symbol_map[param.second] = temp;
        }
        
        if (body) {
            // Function body starts at top level (not inside_label)
            body->generate_code(outcode, local_symbol_map, temp_count, label_count, false);
        }
        
        return "";
    }
};

// Helper class for function arguments

class ArgumentsNode : public ASTNode {
private: 
    vector<ExprNode*> args;

public: 
    ~ArgumentsNode() {}
    
    void add_argument(ExprNode* arg) {
        if (arg) args.push_back(arg);
    }
    
    ExprNode* get_argument(int index) const {
        if (index >= 0 && index < (int)args.size()) {
            return args[index];
        }
        return nullptr;
    }
    
    size_t size() const {
        return args. size();
    }
    
    const vector<ExprNode*>& get_arguments() const {
        return args;
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        return "";
    }
};

// Function call node

class FuncCallNode : public ExprNode {
private:
    string func_name;
    vector<ExprNode*> arguments;

public: 
    FuncCallNode(string name, string result_type)
        : ExprNode(result_type), func_name(name) {}
    
    ~FuncCallNode() {
    }
    
    void add_argument(ExprNode* arg) {
        if (arg) arguments.push_back(arg);
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        vector<string> arg_temps;
        for (auto arg : arguments) {
            string arg_temp = arg->generate_code(outcode, symbol_to_temp, temp_count, label_count, inside_label);
            arg_temps.push_back(arg_temp);
            // Generate param immediately after each argument
            outcode << "param " << arg_temp << endl;
        }
        
        string result_temp = "t" + to_string(temp_count++);
        outcode << result_temp << " = call " << func_name << ", " << arguments.size() << endl;
        
        return result_temp;
    }
};

// Program node (root of AST)

class ProgramNode :  public ASTNode {
private:
    vector<ASTNode*> units;

public: 
    ~ProgramNode() {
        for (auto unit : units) {
            delete unit;
        }
    }
    
    void add_unit(ASTNode* unit) {
        if (unit) units.push_back(unit);
    }
    
    string generate_code(ofstream& outcode, map<string, string>& symbol_to_temp,
                        int& temp_count, int& label_count, bool inside_label = false) const override {
        for (auto unit : units) {
            unit->generate_code(outcode, symbol_to_temp, temp_count, label_count, false);
        }
        return "";
    }
};

#endif
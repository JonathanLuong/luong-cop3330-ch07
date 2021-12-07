/*
 *  UCF COP3330 Fall 2021 Assignment 6 Solution
 *  Copyright 2021 Jonathan Luong   
 */

#include "std_lib_facilities.h"

// Creates token
class Token {
    // kind is always needed while value and name are initialized depending on
    // the needs of each kind. Thus, three distinct constructors are defined.
	char kind;  
	double value;
	string name;

	Token(char ch) :kind{ch} { }
	Token(char ch, double val) :kind{ch}, value{val} { }
	Token(char ch, string n) : kind{ch}, name{n} { }
};

// Models cin as a Token stream
class Token_stream {
public:
    //Token_stream() :full{false}, buffer{0} { }
	Token get();
	void putback(Token t) { buffer.push_back(t); }
	void ignore(char c);
private:
	vector<Token> buffer;
};

// Token kinds
const char let = 'L';
const char quit = 'Q';
const char print = ';';
const char number = '8';
const char name = 'a';
const char sqrtfun = 's';
const char powfun = 'p';
const char pi = 'pi';
const char e = 'e';
const char k = 'k';	
const string declkey = "let";
const string quitkey = "quit";
const string sqrtkey = "sqrt";
const string powkey = "pow";

Token Token_stream::get()
// Processes cin to get tokens from the implemented grammar
{
    // If already have a tokens buffered, return the last one
    if (!buffer.empty()) {
        Token t = buffer.back();
        buffer.pop_back();
        return t;
    }

	char ch;
	cin >> ch;
	switch (ch) {
	case '(':
	case ')':
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case ';':
	case '=':
	case ',':   // Added as separator for function argument lists
		return Token{ch};   // These literals directly define Token kind
	case '.':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':   // Numeric literal
	{	cin.putback(ch);
		double val;
		cin >> val;
		return Token{number, val};
	} 
	default:
	    // We can also expect strings. These to be keywords for declaration and
	    // exiting the program or variable names.
		if (isalpha(ch)) {
			string s;
			s += ch;
			while (cin.get(ch) && (isalpha(ch) || isdigit(ch) || ch == '_')) s += ch;
			cin.putback(ch);
			if (s == declkey) return Token{let};	
			if (s == quitkey) return Token{quit};
			if (s == sqrtkey) return Token{sqrtfun};
			if (s == powkey) return Token{powfun};
			return Token{name,s};
		}
		error("Bad token");
	}
}

void Token_stream::ignore(char c)
// Clear input until c or '\n' is found
{
    // First inspect if a Token of c kind is buffered
    // Eliminate not c kind tokens
    while (!buffer.empty() && buffer.back().kind != c)
        buffer.pop_back();
    // Buffer contains a c kind token
    if (!buffer.empty()) return;

    // and work directly on cin
	char ch{' '};
    while (ch != c && ch != '\n')
        ch = cin.get();
    return;
}

class Variable {
public:
	string name;
	double value;
};

vector<Variable> names;	

double get_value(string s)
{
	for (const Variable& v : names)
	    if (v.name == s) return v.value;
	error("get: undefined name ", s);
}

void set_value(string s, double d)
{
    for (Variable& v : names)
        if (v.name == s) {
            v.value = d;
            return;
        }

	error("set: undefined name ", s);
}

bool is_declared(string s)
{
	for (const Variable& v : names)
	    if (v.name == s) return true;
	return false;
}

Token_stream ts;

double expression();

double eval_function(char c)
// Evaluates function of kind c. The next on input must be "("Expression")".
{
    vector<double> args;    // Vector to store (variable number) of function 
                            // arguments.
    Token t = ts.get();
    if (t.kind != '(') error("'(' expected after function call");

    // Handle argument list. Default: no arguments, do nothing, thus, no
    // default on switch statement.
    switch (c) {
    case sqrtfun:
        args.push_back(expression());
        break;
    case powfun:
        args.push_back(expression());
        t = ts.get();
        if (t.kind != ',') error("Bad number of function arguments");
        args.push_back(expression());
        break;
    }

    t = ts.get();
    if (t.kind != ')') error("Bad number of function arguments");

    // Evaluation snd restrictions implementation
    switch (c) {
    case sqrtfun:
        if (args[0] < 0) error("sqrt() is undefined for negative numbers");
        return sqrt(args[0]);
    case powfun:
        return pow(args[0], narrow_cast<int>(args[1]));
    default:
        // In case we have defined the name as a token for Function rule but
        // forgot to implement its evaluation
        error("Function not implemented");
    }
}

double primary()
{
	Token t = ts.get();
	switch (t.kind) {
	case '(':
	{	double d = expression();
		t = ts.get();
		if (t.kind != ')') error("')' expected");
		return d;
	}
	case '-':   // Negative signed numbers
		return - primary();
	case '+':   // Positive signed numbers
		return primary();
	case number:
		return t.value;
    case name:  // Variable: get value from table
		return get_value(t.name);
	case sqrtfun:
	case powfun:
	    // Call to eval_function by t.kind
	    return eval_function(t.kind);
	default:
		error("primary expected");
	}
}

double term()
{
	double left = primary();
	while(true) {
		Token t = ts.get();
		switch(t.kind) {
		case '*':
			left *= primary();
			break;
		case '/':
		{	double d = primary();
			if (d == 0) error("divide by zero");
			left /= d;
			break;
		}
        case '%':
        {   double d = primary();
            if (d == 0) error("divide by zero");
            left = fmod(left, d);
            break;
        }
		default:
			ts.putback(t);
			return left;
		}
	}
}

double expression()
{
	double left = term();
	while(true) {
		Token t = ts.get();
		switch(t.kind) {
		case '+':
			left += term();
			break;
		case '-':
			left -= term();
			break;
		default:
			ts.putback(t);
			return left;
		}
	}
}

double assignment()
{
    // We get there by knowing that a name and an '=' come next.
    Token t = ts.get();
    string var_name = t.name;
    if (!is_declared(var_name)) error(var_name, " has not been declared");

    ts.get(); // Get rid of the '='
    double d = expression();
    set_value(var_name, d);
    return d;
}


double declaration()
{
    // Check part by part of Declaration gramamr rule behind "let"
	Token t = ts.get();
	if (t.kind != name) error ("name expected in declaration");
	string var_name = t.name;
	if (is_declared(var_name)) error(var_name, " declared twice");

	Token t2 = ts.get();
	if (t2.kind != '=') error("= missing in declaration of " ,var_name);

	double d = expression();
	names.push_back(Variable{var_name,d});
	return d;
}

// Create statement 
double statement()
{
	Token t = ts.get();
	switch(t.kind) {
	case let:
		return declaration();
	case name:
	{
	    Token t2 = ts.get();
	    // Whatever t2 is, we have to rollback
        ts.putback(t2);
        ts.putback(t);
	    if (t2.kind == '=') {
	        return assignment();
	    }
	    return expression();
	}
	default:
		ts.putback(t);
		return expression();
	}
}

void clean_up_mess()
{
	ts.ignore(print);   
}

const string prompt = "> ";
const string result = "= ";


void calculate()
{
	while(true) 
	try {
		cout << prompt;
		Token t = ts.get();
		while (t.kind == print) t = ts.get();   // Remove print commands
		if (t.kind == quit) return;
		ts.putback(t);
		cout << result << statement() << '\n';
	}
	catch(exception& e) {
		cerr << e.what() << '\n';
		clean_up_mess();    // Removes existing data and askes for new
	}
}

int main()
try {
    names.push_back(Variable{"k", 1000});
	names.push_back(Variable{"pi", 3.14159265359, true});
    names.push_back(Variable{"e", 2.71828182846, true});
    
    calculate();
    return 0;
}
catch (exception& e) {
    cerr << "exception: " << e.what() << '\n';
	char c;
	while (cin >> c && c != ';');
    return 1;
}
catch (...) {
    cerr << "Uknown exception!\n";
	char c;
	while (cin >> c && c != ';');
	return 2;
}
#include <iostream>
#include <stdexcept>
#include <string>
#include <map>

// Token stuff
// Token “kind” values:
char const number = '8';    // a floating-point number
char const quit = 'q';      // an exit command
char const print = ';';     // a print command
char const name = 'a';      // a variable name
char const assign = '=';    // an assignment command

std::map<std::string, double> predefinedVals = {
        {"pi", 3.14159},
        {"e", 2.71828}
};

class token
{
    char kind_;       // what kind of token
    double value_;    // for numbers: a value
    std::string name_; // for variables: a name


public:
    // constructors
    token(char ch)
            : kind_(ch)
            , value_(0)
    {
    }
    token(double val)
            : kind_(number)    // let ‘8’ represent “a number”
            , value_(val)
    {
    }
    token(char ch, std::string name)
            : kind_(ch) // let 'ch' represent "a variable"
            , value_(0)
            , name_(name)
    {
    }

    char kind() const
    {
        return kind_;
    }
    double value() const
    {
        return value_;
    }
    std::string name() const
    {
        return name_;
    }

};

// User interaction strings:
std::string const prompt = "> ";
std::string const result = "= ";    // indicate that a result follows

class token_stream
{
    // representation: not directly accessible to users:
    bool full;       // is there a token in the buffer?
    token buffer;    // here is where we keep a Token put back using
    // putback()
public:
    // user interface:
    token get();            // get a token
    void putback(token);    // put a token back into the token_stream
    void ignore(char c);    // discard tokens up to and including a c

    // constructor: make a token_stream, the buffer starts empty
    token_stream()
            : full(false)
            , buffer('\0')
    {
    }
};

// single global instance of the token_stream
token_stream ts;

void token_stream::putback(token t)
{
    if (full)
        throw std::runtime_error("putback() into a full buffer");
    buffer = t;
    full = true;
}

token token_stream::get()    // read a token from the token_stream
{
    // check if we already have a Token ready
    if (full)
    {
        full = false;
        return buffer;
    }

    // note that >> skips whitespace (space, newline, tab, etc.)
    char ch;
    std::cin >> ch;

    switch (ch)
    {
        case '(':
        case ')':
        case ';':
        case 'q':
        case '+':
        case '*':
        case '/':
        case '%':
            return token(ch);// let each character represent itself
        case '-':
        {
            // check if '-' is followed by a number
            if (std::cin.peek() >= '0' && std::cin.peek() <= '9')
            {
                double val;
                std::cin >> val;    // read a floating-point number
                return token(-val); // return negative number
            }
            else
            {
                return token(ch);    // let each character represent itself
            }
        }
        case assign:
            return token(ch);
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            std::cin.putback(ch);    // put digit back into the input stream
            double val;
            std::cin >> val;    // read a floating-point number
            return token(val);
        }
        default:
            if (isalpha(ch))
            {
                if (ch == 'e' || ch == 'q')
                    throw std::runtime_error("Cannot assign to 'e' or 'q'");

                std::string s;
                s += ch;
                while (std::cin.get(ch) && (isalpha(ch) || isdigit(ch) || ch == '_')){
                    s += ch;
                }
                std::cin.putback(ch);
                return token(name, s); // return variable name
            }
            throw std::runtime_error("Bad token");
    }
}

// discard tokens up to and including a c
void token_stream::ignore(char c)
{
    // first look in buffer:
    if (full && c == buffer.kind())    // && means 'and'
    {
        full = false;
        return;
    }
    full = false;    // discard the contents of buffer

    // now search input:
    char ch = 0;
    while (std::cin >> ch)
    {
        if (ch == c)
            break;
    }
}

// declaration so that primary() can call expression()
double expression();

double primary() {
    token t = ts.get();
    switch (t.kind()) {
        case '(': {
            double d = expression();
            t = ts.get();
            if (t.kind() != ')') throw std::runtime_error("')' expected");
            return d;
        }
        case number:
            return t.value();
        case name: {
            token next = ts.get();
            if (next.kind() == assign) {
                double right = expression();
                predefinedVals[t.name()] = right;
                return right;
            } else {
                ts.putback(next);
                return predefinedVals[t.name()];
            }
        }
        default:
            throw std::runtime_error("Primary expected");
    }
}


// exactly like expression(), but for * and /
double term()
{
    double left = primary();    // get the Primary
    while (true)
    {
        token t = ts.get();    // get the next Token ...
        switch (t.kind())
        {
            case '*':
                left *= primary();
                break;
            case '/':
            {
                double d = primary();
                if (d == 0)
                    throw std::runtime_error("divide by zero");
                left /= d;
                break;
            }
            case '%':
            {
                double d = primary();
                if (d == 0)
                    throw std::runtime_error("divide by zero");
                left = fmod(left, d);
                break;
            }
            default:
                ts.putback(t);    // <<< put the unused token back
                return left;      // return the value
        }
    }
}

// read and evaluate: 1   1+2.5   1+2+3.14  etc.
// 	 return the sum (or difference)
double expression()
{
    double left = term();    // get the Term
    token t = ts.get();      // get the next Token
    while (true) {
        switch (t.kind()) {
            case '+':
                left += term();
                t = ts.get();
                break;
            case '-':
                left -= term();
                t = ts.get();
                break;
            default:
                ts.putback(t);
                return left;
        }
    }
}

void clean_up_mess()
{
    ts.ignore(print);
}

void calculate()
{
    while (std::cin)
    {
        try
        {
            std::cout << prompt;    // print prompt
            token t = ts.get();

            // first discard all “prints”
            while (t.kind() == print)
                t = ts.get();

            if (t.kind() == quit)
                return;    // ‘q’ for “quit”

            ts.putback(t);

            std::cout << result << expression() << std::endl;
        }
        catch (std::runtime_error const& e)
        {
            std::cerr << e.what() << std::endl;    // write error message
            clean_up_mess();                       // <<< The tricky part!
        }
    }
}

int main()
{
    try
    {
        calculate();
        return 0;
    }
    catch (...)
    {
        // other errors (don't try to recover)
        std::cerr << "exception\n";
        return 2;
    }
}
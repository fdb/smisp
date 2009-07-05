#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <stack>

#pragma mark Types

class SType;

class SObject {
public:
    SObject();
    SObject(SType *type);
    virtual SObject *eval();
    SType *type();
    virtual std::string toString();
private:
    SType *mType;
};

class SType : public SObject {
public:
    SType(std::string name);
    virtual std::string toString();
    std::string mName;
};

class SName : public SObject {
public:
    SName(std::string name);
    virtual std::string toString();
    std::string mName;
};

class SString : public SObject {
public:
    SString(std::string s);
    virtual std::string toString();
    std::string mValue;
};

class SInt : public SObject {
public:
    SInt(int i);
    virtual std::string toString();
    int mValue;
};

typedef std::list<SObject*> SObjectList;
typedef SObjectList::iterator SObjectListIterator;

class SList : public SObject {
public:
    SList();
    virtual SObject *eval();
    void add(SObject *obj);
    virtual std::string toString();
    SObjectList mList;
};

#pragma mark Builtin Types

SType *SObjectType;
SType *STypeType;
SType *SNameType;
SType *SStringType;
SType *SIntType;
SType *SListType;

#pragma mark Builtin methods

SInt *builtin_plus(SObjectList &args)
{
    if (args.empty()) {
        std::cout << "No arguments";
        return new SInt(0);
    }
    int sum = 0;
    SObjectListIterator it = args.begin();
    // Skip the first element (the name)
    ++it;
    for(; it != args.end(); ++it) {
        SObject *obj = *it;
        SObject *result = obj->eval();
        if (result->type() == SIntType) {
            sum += ((SInt *)result)->mValue;
        } else {
            std::cout << "=ERR= Illegal result type: " << result->toString() << "\n";
            return new SInt(0);
        }
    }
    return new SInt(sum);
}

SObject::SObject()
: mType(SObjectType)
{
}

SObject::SObject(SType *type)
: mType(type)
{
}

std::string SObject::toString()
{
    return "SObject(" + mType->mName + ")";
}

SType *SObject::type()
{
    return mType;
}

SObject *SObject::eval()
{
    return this;
}

SType::SType(std::string name)
:  mName(name)
{
}

std::string SType::toString()
{
    return "SType(" + mName + ")";
}

SName::SName(std::string name)
: SObject(SNameType), mName(name)
{
}

std::string SName::toString()
{
    return mName;
}

SString::SString(std::string s)
: SObject(SStringType), mValue(s)
{
}

std::string SString::toString()
{
    return mValue;
}

SInt::SInt(int i)
: SObject(SIntType), mValue(i)
{
}

std::string SInt::toString()
{
    std::ostringstream o;
    o << mValue;
    return o.str();
}

SList::SList()
: SObject(SListType)
{
}

void SList::add(SObject *obj)
{
    mList.push_back(obj);
}

SObject *SList::eval()
{
    // This is where the magic happens.
    // Look at the first item of the list and see if we can evaluate it:
    // - If it is a list, evaluate that list, recursively.
    // - If it is a name, see if we can call that name with the list as arguments.
    // - Otherwise, just return the list.
    SObject *first = mList.front();
    if (first->type() == SListType) {
        return first->eval();
    } else if (first->type() == SNameType) {
        SName *name = (SName *)first;
        if (name->mName == "+") {
            return builtin_plus(mList);
        } else {
            std::cout << "=ERR= Unknown name '" << name->mName << "'\n";
            return new SString("ERR");
        }
    } else {
        return this;
    }
}

std::string SList::toString()
{
    std::ostringstream o;
    bool first = true;
    SObjectListIterator it;
    o << "(";
    for (it = mList.begin(); it != mList.end(); ++it) {
        if (first) {
            first = false;
        } else {
            o << " ";
        }
        SObject *obj = *it;
        o << obj->toString();
    }
    o << ")";
    return o.str();
}


#pragma mark -
#pragma mark Token

typedef enum {
    TOKEN_WHITESPACE = 0,
    TOKEN_START_BRACKET = 1,
    TOKEN_END_BRACKET = 2,
    TOKEN_NAME = 3,
    TOKEN_NUMBER = 4
} TokenType;

class Token
{
public:
    Token(TokenType type);
    Token(TokenType type, char c);
    void append(char c);
    TokenType mType;
    std::string toString();
    std::string typeName();
    std::string *mContents;
};

Token::Token(TokenType type)
: mType(type), mContents(NULL)
{
}

Token::Token(TokenType type, char c)
: mType(type), mContents(NULL)
{
    mContents = new std::string();
    (*mContents) += c;
}

void Token::append(char c)
{
    (*mContents) += c;
}

std::string Token::typeName()
{
    if (mType == TOKEN_WHITESPACE) {
        return "whitespace";
    } else if (mType == TOKEN_START_BRACKET) {
        return "start bracket";
    } else if (mType == TOKEN_END_BRACKET) {
        return "end bracket";
    } else if (mType == TOKEN_NAME) {
        return "name";
    } else if (mType == TOKEN_NUMBER) {
        return "number";
    } else {
        return "unknown";
    }
}

std::string Token::toString()
{
    if (mContents != NULL) {
        return "Token(" + this->typeName() + ", " + *mContents + ")";
    } else {
        return "Token(" + this->typeName() + ")";
    }
}

typedef std::list<Token*> TokenList;
typedef TokenList::iterator TokenListIterator;

#pragma mark -
#pragma mark main

int main (int argc, char * const argv[]) {
    // Initialize basic runtime types.
    SObjectType = new SType("object");
    STypeType = new SType("type");
    SNameType = new SType("name");
    SStringType = new SType("string");
    SIntType = new SType("int");
    SListType = new SType("list");
    
    // The source code to parse.
    std::string src = "(+ 2 (+ 30 10))";
    std::cout << "Source:\n" << src << "\n\n";
    
    // Tokenize the stream
    std::cout << "Parsing...\n";
    TokenList tokens;
    Token *currentToken = NULL;
    char c;
    for (unsigned int i=0;i<src.length();i++) {
        c = src[i];
        std::cout << c << "\n";
        if (c == '(') {
            currentToken = new Token(TOKEN_START_BRACKET);
            tokens.push_back(currentToken);
        } else if (c == ')') {
            currentToken = new Token(TOKEN_END_BRACKET);
            tokens.push_back(currentToken);
        } else if (c >= '0' && c <= '9') {
            // Already parsing a number
            if (currentToken != NULL && (currentToken->mType == TOKEN_NUMBER || currentToken->mType == TOKEN_NAME)) {
                // Expand the number / name
                currentToken->append(c);
            } else {
                currentToken = new Token(TOKEN_NUMBER, c);
                tokens.push_back(currentToken);
            }
        } else if (c == ' ' || c == '\t' || c == '\n') {
            if (currentToken != NULL && currentToken->mType == TOKEN_WHITESPACE) {
                // Expand the whitespace.
            } else {
                currentToken = new Token(TOKEN_WHITESPACE);
                tokens.push_back(currentToken);
            }
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c<= 'Z') || c == '+') {
            // All name characters.
            if (currentToken != NULL && currentToken->mType == TOKEN_NAME) {
                // Expand the name.
                currentToken->append(c);
            } else {
                currentToken = new Token(TOKEN_NAME, c);
                tokens.push_back(currentToken);
            }
        }
    }
    
    // Show all created tokens.    
    std::cout << "Tokens:\n";
    for (TokenListIterator it = tokens.begin(); it != tokens.end(); ++it) {
        Token *t = *it;
         std::cout << t->toString() << std::endl;
    }
    std::cout << "\n";
    
    // Build the abstract syntax tree.
    std::cout << "Building...\n";
    std::stack<SList*> nodeStack;
    SList *rootList = new SList();
    nodeStack.push(rootList);
    SList *currentList = rootList;
    for (TokenListIterator it = tokens.begin(); it != tokens.end(); ++it) {
        Token *t = *it;
        if (t->mType == TOKEN_WHITESPACE) {
            // Skip.
        } else if (t->mType == TOKEN_START_BRACKET) {
            SList *n = new SList();
            nodeStack.top()->add(n);
            nodeStack.push(n);
            // Go one level deeper.
            currentList = n;
        } else if (t->mType == TOKEN_END_BRACKET) {
            nodeStack.pop();
            currentList = nodeStack.top();
        } else if (t->mType == TOKEN_NAME) {
            SName *n = new SName(*t->mContents);
            currentList->add(n);
        } else if (t->mType == TOKEN_NUMBER) {
            std::istringstream s(*t->mContents);
            int i = 0;
            if (!(s >> i)) {
                std::cout << "=ERR= Bad conversion: " << s << " is not an integer.\n";
            }
            SInt *n = new SInt(i);
            currentList->add(n);
        }
    }
    // Sanity check: is the currentNode back to the rootList?
    if (nodeStack.size() != 1 || rootList != currentList) {
        std::cout << "=ERR= Not enough pops.\n";
    }
    
    std::cout << "\n";
    // Display the root.
    std::cout << "READ: " << rootList->toString() << std::endl;
    // Evaluate the root.
    std::cout << "EVAL: " << rootList->eval()->toString() << std::endl;
    
    return 0;
}

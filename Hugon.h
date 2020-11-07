/* 
 * File:   Hugon.h
 * Author: mkwiatkowski
 *
 * Created on July 21, 2011, 12:01 PM
 */

#ifndef HUGON_H
#define	HUGON_H

#include <string>

static const char *pKeywords[] = {"true","false","null",0};
static const std::string regexpr = "^(-?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)(.*)$";


// virtual base class for event handlers
class HugonHandler {
public:
    HugonHandler() {};
    ~HugonHandler() {};
    virtual void setData(std::string data,bool is_key) = 0;
    virtual void startObject() = 0;
    virtual void endObject() = 0;
    virtual void startArray() = 0;
    virtual void endArray() = 0;
    virtual void startValue() = 0;
    virtual void endValue() = 0;
};

// SAX style JSON parser
class HugonParser {
private:
    std::string mString;
    std::string mToken;
    std::string mStack;
    std::string mLast_token;
    HugonHandler *mHandler;
    
public:
    HugonParser(HugonHandler *pHandler) : mHandler(pHandler) {
        mStack = "?";
    };
    void feed(std::string data);
    void feed(const char *data);
    void close();
private:
    void push_token();
    void send_token(std::string token,bool is_key=false);
    bool get_token();
};

#endif	/* HUGON_H */


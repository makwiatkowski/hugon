#include <string>
#include <cctype>
#include <cstring>
#include "Hugon.h"

#include <iostream>

#ifdef USE_POCO
// http://sourceforge.net/projects/poco/files/sources/poco-1.4.1/poco-1.4.1p1.tar.bz2/download 
#include <Poco/RegularExpression.h>     // Todo - get rid
   using Poco::RegularExpression;
#elif defined USE_BOOST
// http://www.boost.org/
#include <boost/regex.hpp>
   using namespace boost;
#endif

using namespace std;

// Helper functions
bool StringHas(const string &str, const string &chars) {
    for(int idx=0;idx<chars.length();idx++) {
        if(str.find(chars[idx])!=string::npos)
            return true;
    }
    return false;
}

void ltrim( string& str) {
    // Code for Trim Leading Spaces only
    size_t startpos = str.find_first_not_of(" \t\r\n"); // Find the first character position after excluding leading blank spaces
    if( string::npos != startpos )
        str = str.substr( startpos );
}

void rtrim(string &str) {
    // Code for Trim trailing Spaces only
    size_t endpos = str.find_last_not_of(" \t\r\n"); // Find the first character position from reverse af
    if( string::npos != endpos )
        str = str.substr( 0, endpos+1 );
}

void HugonParser::feed(string data) {
    this->mString = this->mString + data;
    while (this->get_token())
        this->push_token();
}

void HugonParser::feed(const char* data) {
    string tmpstr(data);
    this->feed(tmpstr);
}

void HugonParser::close() {
    rtrim(this->mString);
    if (!this->mString.empty() || this->mStack!="?")
        throw "Stream cut";
    this->mToken="";
    this->mLast_token="";
}

void HugonParser::push_token() {
    bool is_key = false;
    // Key end
    if (*(this->mStack.end()-1) == '{' && (this->mToken == "," || this->mToken == "}"))
        if (this->mLast_token != "{")
            this->send_token(")");
    // Null element in array
    if (*(this->mStack.end() - 1) == '[' && (this->mToken == "," || this->mToken == "]"))
        if (this->mToken == "," && (this->mLast_token == "," || this->mLast_token == "["))
            this->send_token("null");
        else if (this->mToken == "]" && this->mLast_token == ",")
            this->send_token("null");
        // Start key
    if (*(this->mStack.end()-1) == '{' and !StringHas(this->mToken, "{}[],:"))
        if (this->mLast_token == "{" || this->mLast_token == ",")
            is_key = true;

    this->mLast_token = this->mToken;
    this->send_token(this->mToken, is_key);
}

void HugonParser::send_token(string token, bool is_key) {
    if (token==",")
        return;

    if (token=="{")
        this->mHandler->startObject();
    else if (token=="}")
        this->mHandler->endObject();
    else if (token=="[")
        this->mHandler->startArray();
    else if (token=="]")
        this->mHandler->endArray();
    else if (token==":")
        this->mHandler->startValue();
    else if (token==")")
        this->mHandler->endValue();
    else {
        if (token[0]=='d')
            token=token.substr(1);
        this->mHandler->setData(token,is_key);
    }
}

bool HugonParser::get_token() {
    ltrim(this->mString);
    rtrim(this->mString);
    
    if (this->mString.empty())
        return false;
    string znak(1,this->mString[0]);

    if (StringHas(znak,"{}[],:")) {
        if (StringHas(znak,"{["))
            this->mStack+=znak;
        else if (StringHas(znak,"}]")) {
            if ((*(this->mStack.end()-1) == '[' && znak=="}") ||
                (*(this->mStack.end()-1) == '{' && znak=="]"))
                    throw "Attribute error";
            this->mStack.erase(this->mStack.end()-1,this->mStack.end());
        }

        this->mToken=znak;
        this->mString=this->mString.substr(1);
        return true;
    }
    
    if (znak=="'" || znak=="\"") {
        bool dc = false;
        for (int idx=1;idx< this->mString.length();idx++) {
            if (dc) {
                dc = false;
                continue;
            }
            if (this->mString[idx]=='\\') {
                dc = true;
                continue;
            }
            if (this->mString[idx]==znak[0]) {
                this->mToken=this->mString.substr(0,idx+1);
                this->mString=this->mString.substr(idx+1);
                return true;
            }
        }
        return false;    // was false
    }
    for (int idx=0;pKeywords[idx];idx++) {
        if (this->mString.find(pKeywords[idx])==0) {
            this->mToken = pKeywords[idx];
            this->mString = this->mString.substr(strlen(pKeywords[idx]));
            return true;
        }
    }
#ifdef USE_POCO
    RegularExpression::MatchVec mtch;
    RegularExpression re(regexpr);
    int result = re.match(this->mString,0,mtch);
    if (result==0)
        return false;
    if (mtch.size()>=5 && mtch[4].length==0)
        return false;
    if (mtch.size()>=5) {
        this->mToken = "d" + this->mString.substr(mtch[1].offset,mtch[1].offset+mtch[1].length);  // was 4
        this->mString = this->mString.substr(mtch[4].offset);
    }
#elif defined  USE_BOOST
    regex expression(regexpr);
    cmatch mtch;
    bool result = regex_match(this->mString.c_str(),mtch,expression);
    if (!result)
        return false;
    this->mToken.assign(mtch[1].first,mtch[1].second);
    this->mToken="d"+this->mToken;
    this->mString = this->mString.substr(strlen(mtch[1].first));
#else
// Wersja bez regex (by ethanak)
    int idx=0;
    // Actually "+" is illegal as first character of number, however
    // ingenuity of programmers is unlimited :D
    if (strchr("+-",this->mString[idx])) idx++;
    while (isdigit(this->mString[idx])) idx++;
    if (this->mString[idx]=='.') {
        idx++;
        while(isdigit(this->mString[idx])) idx++;
    } 
    if (tolower(this->mString[idx])=='e') {
        idx++;
        if (strchr("+-",this->mString[idx])) idx++;
        while(isdigit(this->mString[idx])) idx++;
    };
    this->mToken = "d" + this->mString.substr(0,idx);
    this->mString = this->mString.substr(idx);
#endif


    return true;
}

/* 
 * File:   main.cpp
 * Author: mkwiatkowski
 *
 * This is event driven JSON parser (SAX-style)
 * Code based on hugon.py)
 * 
 * Created on July 18, 2011, 9:42 AM
 * 
 */

// #define USE_BOOST

#include <iostream>
#include <sstream>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>

#include <htmlcxx/html/ParserDom.h>

#include "Hugon.h"

using namespace std;
using namespace htmlcxx;

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::URI;
using Poco::StreamCopier;

enum ELastTag {STRM,TITLE,DURATION,UNKN};

wstring StringToWString(const string &s) {
    wstring temp(s.length(),L' ');
    copy(s.begin(),s.end(),temp.begin());
    return temp;
}

string WStringToString(const wstring& s) {
    string temp(s.length(),' ');
    copy(s.begin(),s.end(),temp.begin());
    return temp;
}

string intToStr(int n)
{
	ostringstream ss;
	ss << n;
	return ss.str();
}

int strToInt(string s)
{
     int ret;
     istringstream iss(s);
     iss >> ret;
     return ret;
}

void subst(string &str, const string &from, const string &to) {
    string::size_type pos = 0;

    while ((pos = str.find(from, pos)) != string::npos) {
        str.replace(pos, from.size(), to);
        pos++;
    }
}

class Item {
public:
    std::wstring title;
    int duration;
    std::string url;
    std::string uri;
    std::string thumbnail;
    bool operator==(Item &itm) {return url==itm.url;};
};

string GetRawData(string url) {
    const string proto("http://");
    if (url.compare(0,proto.size(),proto))
        url = "http://" + url;
    Poco::URI m_Uri(url);
    int redir_cntr=0;
    string outbuf;

follow_redirect_3:
    HTTPClientSession session(m_Uri.getHost(), m_Uri.getPort());
    HTTPRequest req(HTTPRequest::HTTP_GET, m_Uri.getPathAndQuery(), HTTPMessage::HTTP_1_1);
    req.setHost(m_Uri.getHost());
    session.sendRequest(req);
    HTTPResponse res;
    std::istream& rs = session.receiveResponse(res);
    if (res.getStatus() == 302 || res.getStatus() == 301) {
        redir_cntr++;
        m_Uri = res.get("Location");
        if (redir_cntr<4)
            goto follow_redirect_3;
    }

    if (res.getStatus() == 200) {
        StreamCopier::copyToString(rs, outbuf);
    }

    rs.clear();

    return outbuf;

}

vector<string> FindScripts(const char* htmldata) {
    string html(htmldata);

    vector<string> linki;

    HTML::ParserDom parser;
    tree<HTML::Node> dom = parser.parseTree(html);

    // Look for <a .. > tags
    tree<HTML::Node>::iterator it = dom.begin();
    tree<HTML::Node>::iterator end = dom.end();
    int rootdepth = dom.depth(it);

    HTML::Node nod;
    int last_depth = dom.depth(it);
    int cur_depth = last_depth;
    string last_tag="";

    while (it!=end) {
        last_depth = cur_depth;
        cur_depth = dom.depth(it);
        if (it->isTag() &&  it->tagName()=="script") {
            it->parseAttributes();
            last_tag = "script";
        } else if (last_tag=="script") {
            linki.push_back(it->text());
            last_tag ="";
        }
        ++it;
    }
    return linki;
}

class DMHandler : public HugonHandler {
private:
    int level;
    ELastTag eTag;
    string title;
    string duration;
    string url;
public:
    virtual void setData(string data,bool is_key) {
        if (is_key) {
            eTag = UNKN;
            if (data=="\"sdURL\"" ||data=="\"hdURL\"")
                eTag = STRM;
            if (data=="\"videoTitle\"")
                eTag = TITLE;
            if (data=="\"duration\"")
                eTag = DURATION;

        } else {
            if (eTag==STRM) {
                url = data;
                eTag = UNKN;
            } else if (eTag==TITLE) {
                title = data;
                eTag = UNKN;
            } else if (eTag==DURATION) {
                duration = data;
                eTag = UNKN;
            }
        }
    };
    
    string getTitle() { return this->title; };
    string getStream() { return this->url; };
    string getDuration() { return this->duration; };
    
    virtual void startObject() { level++; return; }; // {
    virtual void endObject() { level--; return; };   // }
    virtual void startArray() { level++; return; };  // [
    virtual void endArray() { level--; return; };    // ]
    virtual void startValue() { level++; return; };  // (
    virtual void endValue() { level--; return; };    // )
};


vector<Item> DailyMotionParser(string HtmlData) {
    const string sequence("addVariable(\"sequence\",");
    char *data = new char[HtmlData.length() + 1];
    strcpy(data,HtmlData.c_str());

    vector<Item> items;
    vector<string> linki = FindScripts(data);
    // Print parsed links
    for (int idx=0;idx<linki.size();idx++) {
        DMHandler *pMyDMHandler = new DMHandler();
        int found = linki[idx].find(sequence);
        if (found != string::npos) {
                string rawJSON;
                string Json;
                Item item;

                rawJSON = linki[idx].substr(found+sequence.length());
                found = rawJSON.find("\"");
                rawJSON = rawJSON.substr(found+1);
                found = rawJSON.find("\");");
                rawJSON = rawJSON.substr(0,found);
                URI::decode(rawJSON,Json);
                HugonParser hp(pMyDMHandler);
                hp.feed(Json);
                hp.close();
                item.title = StringToWString(pMyDMHandler->getTitle());
                item.url = pMyDMHandler->getStream();
                item.duration = strToInt(pMyDMHandler->getDuration());
                items.push_back(item);
        }
        delete pMyDMHandler;
    }
    return items;
}



/*
 * 
 */
int main(int argc, char** argv) {
    string HTMLData;
    string url(argv[1]);
    vector<string> scripts;
    vector<Item> items;
    
    HTMLData = GetRawData(url);
    items = DailyMotionParser(HTMLData);
    
    for (int idx=0;idx<items.size();idx++) {
        string title(WStringToString(items[idx].title));
        string url(items[idx].url);
        int duration = items[idx].duration;
        subst(title,"+"," ");
        subst(url,"\\","");
        
        cout << "Track #" << idx << endl;
        cout << "Title    " << title << endl;
        cout << "Stream   " << url << endl;
        cout << "Duration " << duration << endl;
    }
    
    return 0;
}
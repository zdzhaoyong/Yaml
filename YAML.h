#pragma once

#include <iostream>
#include <string>
#include <regex>
#include <iterator>
#include "Jvar.h"

using namespace Py;

class YAML{
public:
    typedef std::pair<std::string,std::string> stringpair;
    YAML(const std::string& context)
        :_context(context){
        _stack=tokenize();
    }

    std::list<stringpair> tokenize(){
        std::string str=std::regex_replace(_context,std::regex("\\r\\n"),"\\n");
        std::smatch captures;
        bool ignore=false;
        int indents = 0, lastIndents = 0,indentAmount = -1;
        stringpair  token;
        std::list<stringpair> stack;
        while (str.length()) {
            for (int i = 0, len = tokens.size(); i < len; ++i)
                if(std::regex_search(str,captures,std::regex(tokens[i].second)))
                {
                    auto type=tokens[i].first;
                    token=std::make_pair(type,captures.str());
                    str = std::regex_replace(str,std::regex(tokens[i].second),"");
                    if(type=="comment")
                    {
                        ignore = true;
                    }
                    else if(type=="indent")
                    {
                        lastIndents = indents;
                        // determine the indentation amount from the first indent
                        if (indentAmount == -1) {
                            indentAmount = token.second.length();
                        }

                        indents = token.second.length() / indentAmount;
                        if (indents == lastIndents)
                            ignore = true;
                        else if (indents > lastIndents + 1)
                            throw -1;
                        else if (indents < lastIndents) {
                            //                      input = token.second.input
                            token.first = "dedent";
                            //                      token.input = input;
                            while (--lastIndents > indents)
                                stack.push_back(token);
                        }
                    }
                    break;
                }
            if (!ignore){
                if (token.first.size()){
                    stack.push_back(token);
                    std::cout<<"push_back:"<<token.first<<","
                            <<token.second<<std::endl;
                    token.first="";
                }
                else
                    throw str;
            }
            ignore = false;
        }
        return stack;
    }

    Svar parse(){
        std::string type=_stack.front().first;
        if("doc"==type) return parseDoc();
        if("-"==type) return parseList();
        if("{"==type) return parseInlineHash();
        if("["==type) return parseInlineList();
        if("id"==type) return parseHash();
        if("string"==type) return advanceValue();
        if("timestamp"==type) return parseTimestamp();
        if("float"==type) return Svar::fromString<double>(advanceValue());
        if("int"==type) return Svar::fromString<int>(advanceValue());
        if("true"==type) {advance();return true;}
        if("false"==type) {advance();return false;}
        if("null"==type) {advance();return Svar::Null();}
        return Svar();
    }

    Svar parseDoc(){
        accept("doc");
        expect("indent", "expected indent after document");
        Svar val = parse();
        return val;
    }

    Svar parseHash() {
        std::string id;
        Svar hash = Svar::Object();
        while (peekType("id") ) {
            id = advanceValue();
            expect(":", "expected semi-colon after id");
            ignoreSpace();
            if (accept("indent")){
                hash.Set(id,parse());
                expect("dedent", "hash not properly dedented");
            }
            else
                hash.Set(id,parse());
            ignoreSpace();
        }
        return hash;
    }

    Svar parseInlineHash() {
      Svar hash = Svar::Object();
      std::string id;
      int i = 0;
      accept("{");
      while (!accept("}")) {
        ignoreSpace();
        if (i) expect(",", "expected comma");
        ignoreWhitespace();
        if (peekType("id")) {
          id = advanceValue();
          expect(":", "expected semi-colon after id");
          ignoreSpace();
          hash.Set(id,parse());
          ignoreWhitespace();
        }
        ++i;
      }
      return hash;
    }

    Svar parseList() {
      Svar list = Svar::Array();
      while (accept("-")) {
        ignoreSpace();
        if (accept("indent")){
          list.classPtr()->_methods["append"](list,parse());
          expect("dedent", "list item not properly dedented");
        }
        else
          list.classPtr()->_methods["append"](list,parse());
        ignoreSpace();
      }
      return list;
    }

    Svar parseInlineList() {
      Svar list = Svar::Array();
      int i = 0;
      accept("[");
      while (!accept("]")) {
        ignoreSpace();
        if (i) expect(",", "expected comma");
        ignoreSpace();
        list=list+parse();
        ignoreSpace();
        ++i;
      }
      return list;
    }

    Svar parseTimestamp() {
      std::string v = _stack.front().second;
      return v;
    }


    bool peekType(std::string type){
        return _stack.front().first==type;
    }

    void ignoreSpace(){
        while(peekType("space")) advance();
    }

    void ignoreWhitespace(){
        while (peekType("space") ||
               peekType("indent") ||
               peekType("dedent"))
            advance();
    }

    bool advance(){
        _stack.pop_front();
        return !_stack.empty();
    }

    std::string advanceValue(){
//        _stack.pop_front();
//        return _stack.front();
        std::string ret=_stack.front().second;
        _stack.pop_front();
        return ret;
    }

    void expect(std::string type,std::string msg){
        if(accept(type)) return;
        throw msg;
    }

    bool accept(std::string type){
        if(peekType(type)) return advance();
        return false;
    }

    std::string _context;
    std::list<stringpair> _stack;
    std::vector<stringpair> tokens={
        {"comment", "^#[^\\n]*"},
        {"indent", "^\\n( *)"},
        {"space", "^ +"},
        {"true", "^\\b(enabled|true|yes|on)\\b"},
        {"false", "^\\b(disabled|false|no|off)\\b"},
        {"null", "^\\b(null|Null|NULL|~)\\b"},
        {"string", "^\"(.*?)\""},
        {"string", "^'(.*?)'"},
        {"timestamp", "^((\\d{4})-(\\d\\d?)-(\\d\\d?)(?:(?:[ \\t]+)(\\d\\d?):(\\d\\d)(?::(\\d\\d))?)?)"},
        {"float", "^(\\d+\\.\\d+)"},
        {"int", "^(\\d+)"},
        {"doc", "^---"},
        {",", "^,"},
        {"{", "^\\{(?![^\\n\\}]*\\}[^\\n]*[^\\s\\n\\}])"},
        {"}", "^\\}"},
        {"[", "^\\[(?![^\\n\\]]*\\][^\\n]*[^\\s\\n\\]])"},
        {"]", "^\\]"},
        {"-", "^\\-"},
        {":", "^[:]"},
        {"string", "^(?![^:\\n\\s]*:[^\\/]{2})(([^:,\\]\\}\\n\\s]|(?!\\n)\\s(?!\\s*?\\n)|:\\/\\/|,(?=[^\\n]*\\s*[^\\]\\}\\s\\n]\\s*\\n)|[\\]\\}](?=[^\\n]*\\s*[^\\]\\}\\s\\n]\\s*\\n))*)(?=[,:\\]\\}\\s\\n]|$)"},
        {"id", "^([\\w][\\w -]*)"}
    };

};

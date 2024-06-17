/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FFORMAT_H
#define FFORMAT_H

//****************************************************************************************************************
//  FFormat v0.9.3
// (c) IGN/LaSTIG
// Author: Christophe.Meynard@ign.fr
//****************************************************************************************************************

#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <stdio.h>

namespace FortranFormat {

enum class FType
{
    T_UNDEF=0, T_H, T_S, T_X, T_DOLLAR, T_COMA, T_SLASH, T_LIST, T_ENDLIST, T_BEGIN, T_END,     // Don't consume Args
    T_A, T_I, T_D, T_E, T_F, T_G, T_L                                                           // Consume Args T_A must be first
};

class FOutFlags
{
public:
    enum Flag {
        F_PLUS = 1, F_COMA = 2,             // Reset at each formatting
        F_NO_NEWLINE = 4, F_NO_THROW_ERROR = 8, F_NO_REPEATLAST = 16, F_NO_STRICT_STRING = 32
    };

    FOutFlags() : flags((Flag)0) {}

    void set(Flag f) { flags = (Flag) (flags | f);}
    void set(Flag f,bool on) { if (on) set(f); else clear(f);}
    void clear(Flag f) { flags = (Flag) (flags & ~f);}
    void reset() { flags = (Flag) (flags & ~(F_PLUS | F_COMA)) ;}
    bool operator()(Flag f) const { return (flags & f);}

private:
    Flag flags;
};


class FSpec
{
public:
    typedef std::vector<FSpec> List;
    typedef List::const_iterator ListIt;

private:
    friend class FFormat;
    friend class FContext;

    FType type() const { return t;}
    std::string name() const;
    unsigned width() const { return w;}
    unsigned repeat() const { return r;}
    int prec() const { return n;}
    int exp() const { return e;}
    unsigned position() const { return pos;}
    const std::string& string() const { return litteral;}
    const List& specList() const { return specs;}

    FType t;
    int r,w,n,e;
    size_t pos;
    std::string litteral;
    List specs;
    FSpec() : t(FType::T_UNDEF),r(-1),w(-1),n(-1),e(-1),pos(-1) {}
};


class FContext
{
public:
    bool flags(FOutFlags::Flag f) const {return oFlags(f);}

    int argNum() const { return argCount;}
    int fmtPos() const { return specIt->position();}

    FType type() const { return specIt->type();}
    int width() const { return specIt->width();}
    int prec() const { return specIt->prec();}
    int exp() const { return specIt->exp();}
    std::string name() const { return specIt->name();}

    std::string format() const { return fmt; }

    void addField(const std::string &field);
    void nextField() { nextField(false);}
private:
    friend class FFormat;

    void clear();
    void reset(const FSpec::ListIt& it);
    void nextField(bool endArgs);

    FSpec::ListIt specIt;
    std::vector<std::pair<FSpec::ListIt, unsigned>> stack;
    FOutFlags oFlags;
    unsigned repeatCount;
    int argCount;
    std::string out;
    std::string fmt;
    bool doNextField;
    bool specRewind;
};

class FormatError : public std::runtime_error
{
public:
    FormatError(const std::string& msg) : runtime_error(msg) {}
};

class ParseError : public FormatError
{
public:
    ParseError(const std::string& msg , int pos, const std::string& fmt) :
        FormatError(msg == "" ? "" : "Fortran format error at pos " + std::to_string(pos+1) + " : " + msg + "\n" + "Format : <" + fmt + ">")
    {}
};

class UsageError : public FormatError
{
public:
    UsageError(const FContext& ctx, const std::string& msg) :
        FormatError("Fortran format usage error: Arg #" + std::to_string(ctx.argNum()) + ", Format pos " + std::to_string(ctx.fmtPos()) + ": " +
                    msg +
                    "\n Format: <" + ctx.format() + ">")
    {}
};


class FFormat {
public:
    explicit FFormat(bool throwException = true);
    FFormat(const std::string& format, bool throwException = true);
    FFormat(const char *format, bool throwException = true) : FFormat(std::string(format),throwException) {}
    bool format(const std::string& format);
    bool ok() const;
    bool parseError() const;
    bool usageError() const;
    std::string errorString() const;
    int predictedLen() const;
    std::string lastString() const;

    void setThrowException(bool on);
    void setRepeatLastSpec(bool on);
    void setStrictString(bool on);
    void setNoEndingEOL(bool on);
    void setMaxOutputLen(int maxLen);

    template <typename... Args>
    std::string toString(Args&&... args);

    template <typename... Args>
    int print(Args&&... args);

    template <typename... Args>
    int fprint(::FILE *f, Args&&... args);

    template <typename... Args>
    int snprint(char *s, int len, Args&&... args);

    template <typename... Args>
    std::string toStringf(const char* fmt, Args&&... args);

    template <typename... Args>
    int printf(const char* fmt, Args&&... args);

    template <typename... Args>
    int fprintf(::FILE *f, const char* fmt, Args&&... args);

    template <typename... Args>
    int snprintf(char *s, int len, const char* fmt, Args&&... args);


private:
    typedef std::vector<FSpec> FSpecs;
    typedef FSpecs::const_iterator FSpecIt;

    int get();
    void put();
    void space();
    bool acceptChar(int c);
    bool acceptInt(int *n);
    bool acceptPositive(int *n);
    char acceptNoRepeat(FSpec& spec, char c);
    bool acceptX(FSpec &spec);
    bool acceptSlash(FSpec &spec);
    bool acceptDollar(FSpec &spec);
    bool acceptS(FSpec &spec);
    bool acceptDC(FSpec &spec);
    bool acceptLitSpec(FSpec &spec);
    bool acceptHolleritSpec(FSpec &spec);
    bool acceptStringSpec(FSpec &spec);
    bool acceptLogicalSpec(FSpec &spec);
    bool acceptIntSpec(FSpec &spec);
    bool acceptRealFSpec(FSpec &spec);
    bool acceptRealDEGSpec(FSpec &spec);
    bool acceptItem(FSpec &spec);
    bool acceptList(FSpecs &specList, int end, int& predictedLen);

    void doFormat();

    template <typename Head, typename... Tail>
    void doFormat(Head const& head, Tail&&...tail);

    FormatError error;
    std::string fmt;
    size_t pos;
    FSpecs *specs;
    int outLen;
    int maxOutputLen;

    FContext ctx;

    std::unordered_map<std::string, std::pair<FSpecs, int>> cache;
};



// Implementation


template<typename T> struct always_false : std::false_type {};

template<typename FFArg_Not_Defined_For_This_Type>
void FFArg(FContext&, const FFArg_Not_Defined_For_This_Type&)
{
    static_assert(always_false<FFArg_Not_Defined_For_This_Type>::value, "FortranFormat: FFArg(const FContext&,const T&) not defined for this type (see above messages)");
}

void FFArg(FContext&, const bool &);

void FFArg(FContext&, const short int &);
void FFArg(FContext&, const short unsigned &);
void FFArg(FContext&, const int &);
void FFArg(FContext&, const unsigned &);
void FFArg(FContext&, const long int &);
void FFArg(FContext&, const long unsigned &);
void FFArg(FContext&, const long long int &);
void FFArg(FContext&, const long long unsigned &);

void FFArg(FContext&, const signed char &);
void FFArg(FContext&, const unsigned char &);

void FFArg(FContext&, const std::string &);
void FFArg(FContext&, const char *);

void FFArg(FContext&, const float &);
void FFArg(FContext&, const double &);
void FFArg(FContext&, const long double &);


inline void FFArg(FContext &ctx, const  std::vector<int> &intArray)
{
    for (const auto&i : intArray )
        FFArg(ctx,i);
}

inline void FFArg(FContext& ctx, const char &v)
{
    if (std::is_same<char,signed char>::value)
        FFArg(ctx,(signed char) v);
    else
        FFArg(ctx,(unsigned char) v);
}

template <typename Head, typename... Tail>
void FFormat::doFormat(Head const& head, Tail&&...tail)
{
    ctx.argCount++;
    FFArg(ctx, head);
    this->doFormat(std::forward<Tail>(tail)...);
}


template <typename... Args>
std::string FFormat::toString(Args&&... args)
{
    if (parseError())
        return "";

    ctx.reset(specs->begin());
    if (! ctx.flags(FOutFlags::F_NO_THROW_ERROR)) {
        doFormat(std::forward<Args>(args)...);
    } else {
        try {
            doFormat(std::forward<Args>(args)...);
        } catch(UsageError& e) {
            error = e;
            ctx.out = "";
        }
    }
    if (maxOutputLen > 0 && ctx.out.length() > (unsigned)maxOutputLen) {
        error = UsageError(ctx,"Max output len exceeded");
        ctx.out="";
        if (! ctx.flags(FOutFlags::F_NO_THROW_ERROR))
            throw error;
    }
    return lastString();
}

template <typename... Args>
int FFormat::print(Args&&... args)
{
    toString(args...);
    if (! ok())
        return -1;
    return ::printf("%s",lastString().c_str());
}

template <typename... Args>
int FFormat::fprint(::FILE *f, Args&&... args)
{
    toString(args...);
    if (! ok())
        return -1;
    return ::fprintf(f,"%s",lastString().c_str());
}

template <typename... Args>
int FFormat::snprint(char *s, int len, Args&&... args)
{
    toString(args...);
    if (! ok())
        return -1;
    return ::snprintf(s,len,"%s",lastString().c_str());
}

///////////////////////////////////////////////////
template <typename... Args>
std::string FFormat::toStringf(const char* fmt, Args&&... args)
{
    format(fmt);
    return toString(args...);
}

template <typename... Args>
int FFormat::printf(const char* fmt, Args&&... args)
{
    format(fmt);
    return print(args...);
}

template <typename... Args>
int FFormat::fprintf(::FILE *f, const char* fmt, Args&&... args)
{
    format(fmt);
    return fprint(f,args...);
}

template <typename... Args>
int FFormat::snprintf(char *s, int len, const char* fmt, Args&&... args)
{
    format(fmt);
    return snprint(s,len,args...);
}

///////////////////////////////////////////////////

template <typename... Args>
inline std::string ToStringf(const char* fmt, Args&&... args)
{
    FFormat format(fmt);
    return format.toString(args...);
}

template <typename... Args>
inline int printf(const char* fmt, Args&&... args)
{
    FFormat format(fmt);
    return format.print(args...);
}

template <typename... Args>
int fprintf(::FILE *f, const char* fmt, Args&&... args)
{
    FFormat format(fmt);
    return format.fprint(f,args...);
}

template <typename... Args>
int snprintf(char *s, int len, const char* fmt, Args&&... args)
{
    FFormat format(fmt);
    return format.snprint(s,len,args...);
}



} // namespace FortranFormat
#endif // FFORMAT_H

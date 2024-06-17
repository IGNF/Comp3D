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

#include "fformat.h"
#include <iostream>
#include <cmath>

namespace FortranFormat {

////////////////////////////////////////////////////////////////
//  General
////////////////////////////////////////////////////////////////

FFormat::FFormat(bool throwException) : error("Unspecified format"),
    pos(0), specs(nullptr), outLen(0), maxOutputLen(-1)
{
    ctx.clear();
    ctx.oFlags.set(FOutFlags::F_NO_THROW_ERROR, !throwException);
}

FFormat::FFormat(const std::string &fmt, bool throwException) :
    error("Unspecified format"), pos(0), specs(nullptr), outLen(0), maxOutputLen(-1)
{
    ctx.clear();
    ctx.oFlags.set(FOutFlags::F_NO_THROW_ERROR, !throwException);
    format(fmt);
}

bool FFormat::ok() const
{
    return error.what()[0] == 0;
}

bool FFormat::parseError() const
{
    return dynamic_cast<const ParseError*>(&error) != 0;
}

bool FFormat::usageError() const
{
    return dynamic_cast<const UsageError*>(&error) != 0;
}

std::string FFormat::errorString() const
{
    return error.what();
}

int FFormat::predictedLen() const
{
    return outLen;
}

std::string FFormat::lastString() const
{
    return ctx.out;
}

void FFormat::setThrowException(bool on)
{
    ctx.oFlags.set(FOutFlags::F_NO_THROW_ERROR, !on);
}


void FFormat::setRepeatLastSpec(bool on)
{
    ctx.oFlags.set(FOutFlags::F_NO_REPEATLAST, !on);
}

void FFormat::setStrictString(bool on)
{
    ctx.oFlags.set(FOutFlags::F_NO_STRICT_STRING, !on);
}

void FFormat::setNoEndingEOL(bool on)
{
    ctx.oFlags.set(FOutFlags::F_NO_NEWLINE, !on);
}

void FFormat::setMaxOutputLen(int maxLen)
{
    maxOutputLen = maxLen;
}




////////////////////////////////////////////////////////////////
//  Parsing format specifier
////////////////////////////////////////////////////////////////

inline int FFormat::get()
{
    if (pos >= fmt.size()) {
        pos++;
        return -1;
    }
    return fmt[pos++];
}

inline void FFormat::put()
{
    pos--;
}

inline void FFormat::space()
{
    if (get() == ' ') {
        while (get() == ' ');
    }
    put();
}

inline bool FFormat::acceptChar(int c)
{
    if (c == toupper(get()))
        return true;
    put();
    return false;
}


bool FFormat::acceptInt(int  *n)
{
    int p=pos;
    char c = get();
    if (c < '0' || c > '9') {
        put();
        return false;
    }
    *n=0;
    do {
        if (*n > 10000)
            throw ParseError(std::string("Too big number in specifier."), p,fmt);
        *n = *n * 10 + c -'0';
        c = get();
    } while (c >= '0' && c <= '9');
    put();
    return true;
}


inline char FFormat::acceptNoRepeat(FSpec& spec, char c)
{
    if (! acceptChar(c))
        return false;
    if (spec.r >= 0)
        throw ParseError(std::string("Format '") + c + "' must not have a repeat prefix.",pos,fmt);
    spec.r = 0;
    return c;
}

inline bool FFormat::acceptPositive(int *n)
{
    if (!acceptInt(n))
        return false;
    if (*n == 0)
        throw ParseError("Expecting non zero integer.",pos,fmt);
    return true;
}


bool FFormat::acceptX(FSpec &spec)
{
    if (! acceptChar('X'))
        return false;
    if (spec.r < 0)
        throw ParseError("Format 'X' must be preceded by a non zero integer.",pos,fmt);
    spec.t = FType::T_X;
    spec.w = 1;
    return true;
}

bool FFormat::acceptSlash(FSpec &spec)
{
    if (! acceptChar('/'))
        return false;
    if (spec.r < 0)
        spec.r = 1;
    spec.t = FType::T_SLASH;
    spec.w = 1;
    return true;
}

bool FFormat::acceptS(FSpec &spec)
{
    if (! acceptNoRepeat(spec,'S'))
        return false;
    spec.t = FType::T_S;
    if (acceptChar('S')) {
        spec.n = 0;
        return true;
    }
    spec.n = 1;
    acceptChar('P');
    return true;
}

bool FFormat::acceptDC(FSpec &spec)
{
    int on;
    if (! acceptChar('D'))
        return false;
    if (acceptChar('P'))
        on = 0;
    else if (acceptChar('C')) {
        on = 1;
    } else {
        put();              // unget the initial 'D'
        return false;
    }
    if (spec.r >= 0)
        throw ParseError(std::string("Format 'D") + (on ? 'C' : 'P') + "' must not have a repeat prefix.",pos,fmt);
    spec.t = FType::T_COMA;
    spec.n = on;
    spec.r = 0;
    return true;
}

bool FFormat::acceptDollar(FSpec &spec)
{
    if (! acceptNoRepeat(spec,'$'))
        return false;
    spec.t  = FType::T_DOLLAR;
    ctx.oFlags.set(FOutFlags::F_NO_NEWLINE);
    return true;
}


bool FFormat::acceptLitSpec(FSpec &spec)
{
    char delim;

    if (acceptNoRepeat(spec,'\''))
        delim = '\'';
    else if (acceptNoRepeat(spec,'"'))
        delim = '"';
    else
        return false;

    spec.r = 1;
    spec.t = FType::T_H;
    spec.litteral = "";
    while (true) {
        char c = get();
        if (c == -1)
            throw ParseError("Unterminated string litteral.",spec.pos,fmt);
        if (c == delim && get() != delim)  {
            put();
            spec.w = spec.litteral.length();
            return true;
        }
        spec.litteral += c;
    }
}

bool FFormat::acceptHolleritSpec(FSpec &spec)
{
    if (! acceptChar('H'))
        return false;
    if (spec.r < 0)
        throw ParseError("Format 'H' must be preceded by a non zero positive integer.",pos,fmt);
    spec.t = FType::T_H;
    spec.litteral = "";
    for (int i=0; i<spec.r; i++) {
        char c = get();
        if (c == -1)
            throw ParseError("Unterminated H litteral.",spec.pos,fmt);
        spec.litteral += c;
    }
    spec.w = spec.r;
    spec.r = 1;
    return true;
}

bool FFormat::acceptStringSpec(FSpec &spec)
{
    if (! acceptChar('A'))
        return false;
    spec.t = FType::T_A;
    if (! acceptPositive(&spec.w))
        throw ParseError("Expecting a non zero positive integer.",pos,fmt);
    if (spec.r < 0)
        spec.r = 1;
    if (! acceptChar('.'))
        return true;
    int p=pos;
    if (! acceptInt(&spec.n))
        throw ParseError("Expecting a zero or positive integer.",p,fmt);
    if (spec.n > spec.w)
        throw ParseError(std::string("m (") + std::to_string(spec.n) + ") must be equal or less than w (" + std::to_string(spec.w) + ").",p,fmt);
    return true;
}


bool FFormat::acceptLogicalSpec(FSpec &spec)
{
    if (! acceptChar('L'))
        return false;
    spec.t = FType::T_L;
    if (! acceptPositive(&spec.w))
        throw ParseError("Expecting a non zero positive integer.",pos,fmt);
    if (spec.r < 0)
        spec.r = 1;
    return true;
}

bool FFormat::acceptIntSpec(FSpec &spec)
{
    if (! acceptChar('I'))
        return false;
    spec.t = FType::T_I;
    if (! acceptPositive(&spec.w))
        throw ParseError("Expecting a non zero positive integer.",pos,fmt);
    if (spec.r < 0)
        spec.r = 1;
    if (! acceptChar('.'))
        return true;
    int p=pos;
    if (! acceptInt(&spec.n))
        throw ParseError("Expecting a zero or positive integer.",p,fmt);
    if (spec.n > spec.w)
        throw ParseError(std::string("m (") + std::to_string(spec.n) + ") must be equal or less than w (" + std::to_string(spec.w) + ").",p,fmt);
    return true;
}

bool FFormat::acceptRealFSpec(FSpec &spec)
{
    if (!acceptChar('F'))
        return false;

    spec.t = FType::T_F;
    int p=pos;
    if (! acceptInt(&spec.w))
        throw ParseError("Expecting a zero or positive integer.",pos,fmt);
    if (spec.r < 0)
        spec.r = 1;

    if (! acceptChar('.'))
        throw ParseError("Expecting '.' .",spec.pos,fmt);

    if (! acceptInt(&spec.n))
        throw ParseError("Expecting a zero or positive integer.",pos,fmt);
    if (spec.w == 0)
        spec.w = spec.n+1;
    else if (spec.w < spec.n + 2)
            throw ParseError(std::string("w (") + std::to_string(spec.w) + ") must be greater or equal han n+2 (" + std::to_string(spec.n+2) + ").",p,fmt);
    return true;
}

bool FFormat::acceptRealDEGSpec(FSpec &spec)
{
    if (acceptChar('D'))
        spec.t = FType::T_D;
    else if (acceptChar('E'))
        spec.t = FType::T_E;
    else if (acceptChar('G'))
        spec.t = FType::T_G;
    else
        return false;

    int p=pos;
    if (! acceptPositive(&spec.w))
        throw ParseError("Expecting a non zero positive integer.",pos,fmt);
    if (spec.r < 0)
        spec.r = 1;

    if (! acceptChar('.')) {
        if (spec.t == FType::T_G)
            return true;
        throw ParseError("Expecting '.' .",spec.pos,fmt);
    }
    if (! acceptInt(&spec.n))
        throw ParseError("Expecting a zero or positive integer.",pos,fmt);

    if (spec.t == FType::T_D) {
        if (spec.w < spec.n + 6)
            throw ParseError(std::string("w (") + std::to_string(spec.w) + ") must be greater or equal han n+6 (" + std::to_string(spec.n+6) + ").",p,fmt);
        return true;
    }
    if (!acceptChar('E')) {
        if (spec.t == FType::T_G) {
            if (spec.w < spec.n)
                throw ParseError(std::string("w (") + std::to_string(spec.w) + ") must be greater or equal han n (" + std::to_string(spec.n) + ").",p,fmt);
        } else {
            if (spec.w < spec.n + 6)
                throw ParseError(std::string("w (") + std::to_string(spec.w) + ") must be greater or equal han n+6 (" + std::to_string(spec.n+6) + ").",p,fmt);
        }
        return true;
    }
    int pe=pos;
    if (! acceptPositive(&spec.e))
        throw ParseError("Expecting a non zero positive integer.",pos,fmt);
    if (spec.e > 20)
        throw ParseError("e value is to big (max: 20)",pe,fmt);
    if (spec.w < spec.n + spec.e + 4)
        throw ParseError(std::string("w (") + std::to_string(spec.w) + ") must be greater or equal than n+e+4 (" + std::to_string(spec.n+spec.e+4) + ").",p,fmt);
    return true;
}

bool FFormat::acceptItem(FSpec &spec)
{
    spec.pos = pos;
    if (acceptX(spec))
        return true;
    if (acceptS(spec))
        return true;
    if (acceptDC(spec))                 // Must be called before acceptRealSpec
        return true;
    if (acceptSlash(spec))
        return true;
    if (acceptDollar(spec))
        return true;
    if (acceptLitSpec(spec))
        return true;
    if (acceptHolleritSpec(spec))
        return true;
    if (acceptLogicalSpec(spec))
        return true;
    if (acceptStringSpec(spec))
        return true;
    if (acceptIntSpec(spec))
        return true;
    if (acceptRealFSpec(spec))
        return true;
    if (acceptRealDEGSpec(spec))
        return true;
    if (!acceptChar('('))
        throw ParseError("Expecting format specifier.",pos,fmt);
    spec.t = FType::T_LIST;
    if (spec.r < 0)
        spec.r = 1;
    space();
    return acceptList(spec.specs,')',spec.w);
}


bool FFormat::acceptList(FSpecs &specList, int end, int& len)
{
    len = 0;
    while(true) {
        FSpec spec;
        space();
        acceptPositive(&spec.r);
        if (! acceptItem(spec))
            throw ParseError("Expecting format specifier.",pos,fmt);
        len += spec.r * spec.w;
        specList.push_back(spec);
        space();
        if (acceptChar(end)) {
            spec.t = end == -1 ? FType::T_END : FType::T_ENDLIST;
            spec.r = spec.w = 0;
            spec.pos = pos;
            specList.push_back(spec);
            return true;
        }
        if (acceptChar(','))
            continue;
        if (spec.t == FType::T_SLASH)
            continue;
        if (acceptChar('/')) {
            put();
            continue;
        }
        if (end != -1)
            throw ParseError(std::string("',' or '") + ((char)end) + "' expected." ,pos,fmt);
        else
            throw ParseError("',' expected.",pos,fmt);
    }
}

bool FFormat::format(const std::string &format)
{
    fmt = format;
    error = FormatError("");

    auto inserted = cache.emplace(format,std::make_pair(FSpecs(),0));
    specs = &inserted.first->second.first;
    if (! inserted.second) {
        outLen = inserted.first->second.second;
        pos = format.length();
        return true;
    }

    ctx.clear();
    ctx.fmt = fmt;
    ctx.oFlags.clear(FOutFlags::F_NO_NEWLINE);

    pos = 0;

    FSpec spec;
    spec.t = FType::T_BEGIN;
    spec.r = 1;
    specs->push_back(spec);
    if (! ctx.oFlags(FOutFlags::F_NO_THROW_ERROR))  {
        acceptList(*specs,-1,outLen);
    } else {
        try {
            acceptList(*specs,-1,outLen);
        } catch (ParseError& e) {
            error = e;
            return false;
        }
    }
    if (! ctx.oFlags(FOutFlags::F_NO_NEWLINE))
        outLen++;

    inserted.first->second.second = outLen;

    return true;
}

////////////////////////////////////////////////////////////////
//  Applying format
////////////////////////////////////////////////////////////////

// BUG: Ne pas faire la sortie des formats suivants si plus d'arguments !
void FFormat::doFormat()
{
    ctx.nextField(true);
    if (ctx.specIt->type() != FType::T_END && !ctx.specRewind)
        throw UsageError(ctx,"Remaining format specifiers but no more args.");
    if (! ctx.oFlags(FOutFlags::F_NO_NEWLINE))
        ctx.out += '\n';
}


void FFArg(FContext &ctx, const std::string &v)
{
    ctx.nextField();
    if ( ctx.type() != FType::T_A && ctx.type() != FType::T_G)
        throw UsageError(ctx,"Arg is of type string but format specifier is " + ctx.name());
    if (ctx.prec()<0 || (int)v.length() >= ctx.prec())
        return ctx.addField(v);
    ctx.addField(v + std::string(ctx.prec()-v.length(),' '));
}


void FFArg(FContext &ctx, const char *v)
{
    ctx.nextField();
    FFArg(ctx, std::string(v));
}


static void format_I(FContext &ctx, long long unsigned v, bool isSigned)
{
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    if (ctx.type() == FType::T_L )
        return FFArg(ctx, v != 0);
    if (ctx.type() != FType::T_I  && ctx.type() != FType::T_G )
        throw UsageError(ctx,"Arg is of type int but format specifier is " + ctx.name());

    bool neg = false;
    std::string out;

    if (isSigned) {
        neg = ((long long int)v) < 0;
        out = std::to_string(std::abs((long long int)v));
    } else {
        out = std::to_string(v);
    }
    unsigned len = out.length();
    if (neg || ctx.flags(FOutFlags::F_PLUS))
        len++;
    if (ctx.prec() == 0 && v==0)
        return ctx.addField("");
    if (ctx.prec() >= 0 && len < (unsigned)ctx.prec())
        out = std::string(ctx.prec() - len,'0') + out;
    if (neg)
        out = '-' + out;
    else if (ctx.flags(FOutFlags::F_PLUS))
        out = '+' + out;
    ctx.addField(out);

}

void FFArg(FContext &ctx, const long long int &v)
{
    ctx.nextField();
    format_I(ctx,(long long unsigned)v,true);
}

void FFArg(FContext &ctx, const long long unsigned &v)
{
    ctx.nextField();
    format_I(ctx,v,false);
}

void FFArg(FContext &ctx, const signed char &v)
{
    ctx.nextField();
    if (ctx.type() ==FType::T_A)
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long int) v);
}

void FFArg(FContext &ctx, const unsigned char &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A)
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long unsigned) v);
}

void FFArg(FContext &ctx, const short int &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long int) v);
}

void FFArg(FContext &ctx, const short unsigned &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long unsigned) v);
}

void FFArg(FContext &ctx, const int &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long int) v);
}

void FFArg(FContext &ctx, const unsigned  &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long unsigned) v);
}

void FFArg(FContext &ctx, const long int &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long int) v);
}

void FFArg(FContext &ctx, const long unsigned &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx, (long long unsigned) v);
}

void FFArg(FContext &ctx, const bool &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    if (ctx.type() == FType::T_I)
        return FFArg(ctx,(long long unsigned)v);
    if (ctx.type() != FType::T_L  && ctx.type() != FType::T_G)
        throw UsageError(ctx,"Arg is of type bool but format specifier is " + ctx.name());
    ctx.addField (v ? "T" : "F");
}

static void format_F(FContext &ctx, int prec, const long double &v, const std::string& fillRight)
{
    if (std::isnan(v))
        return ctx.addField("NaN" + fillRight);
    if (std::isinf(v)) {
        if (v < 0)
            return ctx.addField("-Inf" + fillRight);
        if (ctx.flags(FOutFlags::F_PLUS))
            return ctx.addField("+Inf" + fillRight);
        return ctx.addField("Inf" + fillRight);
    }

    if (prec < 0)
        prec = 0;
    const char *fmt = "%.*Lf";
    if (v>=0 && ctx.flags(FOutFlags::F_PLUS))
        fmt =  "%+.*Lf";
    size_t size = ::snprintf(NULL,0,fmt,prec,v);
    auto outc = std::vector<char>(size+1);
    ::snprintf(outc.data(),size+1,fmt,prec,v);

    char sep = ctx.flags(FOutFlags::F_COMA) ? ',' : '.';
    for (char *c = outc.data(); *c!=0; c++) {
        if (*c == '.' || *c == ',' ) {
            *c = sep;
            break;
        }
    }
    std::string out(outc.data());

    if (prec == 0)
        out += sep;
    if (prec > 0 && (int)ctx.width() == prec+1) {
        if ((out[0] == '+' || out[0]=='-') && out[1]=='0' && out [2] == sep)
            out = out[0] + out.substr(2);
        else if (out[0]=='0' && out [1] == sep)
            out = out.substr(1);
    }
    ctx.addField(out + fillRight);
}

static void format_E(FContext &ctx, const long double &v)
{
    if (std::isnan(v))
        return ctx.addField("NaN");
    if (std::isinf(v)) {
        if (v < 0)
            return ctx.addField("-Inf");
        if (ctx.flags(FOutFlags::F_PLUS))
            return ctx.addField("+Inf");
        return ctx.addField("Inf");
    }

    const char *fmt = " %.*LE";
    if (v>=0 && ctx.flags(FOutFlags::F_PLUS))
        fmt =  " %+.*LE";
    size_t size = ::snprintf(NULL,0,fmt,ctx.prec()>0 ? ctx.prec() : 1,v);
    auto outc = std::vector<char>(size+1);
    ::snprintf(outc.data(),size+1,fmt,ctx.prec()>0 ? ctx.prec()-1 : 0,v);

    char *c = outc.data();
    while (c[1] <'0' || c[1] >'9')  {
        c[0] = c[1];
        c++;
    }
    c[0] = '0';
    c[2] = c[1];
    c[1] = ctx.flags(FOutFlags::F_COMA) ? ',' : '.';
    if (ctx.prec() <= 0)
        c[2] = 0;
    c += 3;
    while (*c >= '0' && *c <= '9') c++;
    if (*c=='E') {
        *c = 0;
        c++;
    }
    int exp = atoi(c);
    *c = 0;
    if (v != 0)
        exp += 1;

    std::string out;
    if ((int)ctx.width() == ctx.prec() + 6 && (v<0 || ctx.flags(FOutFlags::F_PLUS))) {
        outc[1] = outc[0];
        out = std::string(outc.data()+1);
    } else {
        out = std::string(outc.data());
    }

    char sexp[20 + 4];
    if (ctx.exp() < 0) {
        if (std::abs(exp) > 99)
            ::snprintf (sexp,sizeof(sexp)-1,"%+04d",exp);
        else
            ::snprintf (sexp,sizeof(sexp)-1,"%c%+03d",ctx.type()== FType::T_D ? 'D' : 'E',exp);
    } else {
        if (::snprintf (sexp,sizeof(sexp)-1,"E%+0*d",ctx.exp()+1,exp) > ctx.exp()+2)
            return ctx.addField(std::string(ctx.width(),'*'));
    }
    ctx.addField(out + sexp);
}


void FFArg(FContext &ctx, const long double &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));

    if (ctx.type() == FType::T_F)
        return format_F(ctx,ctx.prec(),v,"");
    if (ctx.type() == FType::T_E || ctx.type() == FType::T_D)
        return format_E(ctx,v);
    if (ctx.type() != FType::T_G)
        throw UsageError(ctx,"Arg is of type real but format specifier is " + ctx.name());
    int exp = std::log10(v);
    if (std::abs(v) < 0.1 || exp > ctx.prec())
        return format_E(ctx,v);
    format_F(ctx,ctx.prec()-exp,v,std::string(ctx.exp() < 0 ? 4 : ctx.exp()+2,' '));
}

void FFArg(FContext &ctx, const double &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx,(long double)v);
}

void FFArg(FContext &ctx, const float &v)
{
    ctx.nextField();
    if (ctx.type() == FType::T_A && ctx.flags(FOutFlags::F_NO_STRICT_STRING))
        return FFArg(ctx, std::string((const char *)(&v), sizeof v));
    FFArg(ctx,(long double)v);
}

std::string FSpec::name() const
{
    switch (type()) {
    case FType::T_UNDEF: return ("<NULL>");
    case FType::T_A: return ("A");
    case FType::T_I: return ("I");
    case FType::T_D: return ("D");
    case FType::T_E: return ("E");
    case FType::T_F: return ("F");
    case FType::T_G: return ("G");
    case FType::T_L: return ("L");
    case FType::T_LIST: return ("(");
    case FType::T_ENDLIST: return (")");
    case FType::T_H: return ("'");
    case FType::T_X: return ("X");
    case FType::T_SLASH: return ("/");
    case FType::T_DOLLAR: return ("$");
    case FType::T_S: return ("S");
    case FType::T_COMA: return ("DC");
    case FType::T_BEGIN: return ("<BEGIN>");
    case FType::T_END: return ("<END>");
    }
    return "";
}


void FContext::clear()
{
    argCount = 0;
    oFlags.reset();
    out = "";
    specRewind = false;
    doNextField = true;
}


void FContext::reset(const FSpec::ListIt& it)
{
    clear();
    specIt = it;
}


void FContext::addField(const std::string &field)
{
    if (field.length() > specIt->width()) {
        if (specIt->type() == FType::T_A)
            out += field.substr(0,specIt->width());
        else
            out += std::string(specIt->width(),'*');
    } else {
        out += std::string(specIt->width() - field.length(),' ');
        out += field;
    }
    doNextField = true;
}

void FContext::nextField(bool endArgs)
{
    if (! doNextField)
        return;
    doNextField = false;
    if (++repeatCount < specIt->repeat() )
        return;
    ++specIt;
    while (true) {
        switch (specIt->type()) {
        case FType::T_BEGIN:
        case FType::T_UNDEF:
            throw UsageError(*this,"Internal error, Invalid format specifier " + std::string(specIt->name()));
        case FType::T_A:
        case FType::T_I:
        case FType::T_D:
        case FType::T_E:
        case FType::T_F:
        case FType::T_G:
        case FType::T_L:
            repeatCount = 0;
            return;
        case FType::T_END:
            if (endArgs)
                return;
            if (oFlags(FOutFlags::F_NO_REPEATLAST))
                throw UsageError(*this,"Too many arguments for format");
            if ((specIt-1)->type() < FType::T_A && (specIt-1)->type() != FType::T_LIST )
                throw UsageError(*this,"Too many arguments for format");
            specRewind = true;
            --specIt;
            if (specIt->type() == FType::T_LIST)
                continue;
            return;
        case FType::T_LIST:
            stack.push_back(make_pair(specIt,0));
            specIt = specIt->specList().begin();
            continue;
        case FType::T_ENDLIST:
        {
            auto pair = stack.back();
            specIt = pair.first;
            repeatCount = pair.second;
        }
            stack.pop_back();
            if (++repeatCount >= specIt->repeat() ) {
                ++specIt;
            } else {
                stack.push_back(make_pair(specIt,repeatCount));
                specIt = specIt->specList().begin();
            }
            continue;
        case FType::T_H:
            out += specIt->string();
            break;
        case FType::T_X:
            out += std::string(specIt->repeat(), ' ');
            break;
        case FType::T_SLASH:
            out += std::string(specIt->repeat(), '\n');
            break;
        case FType::T_DOLLAR:
            oFlags.set(FOutFlags::F_NO_NEWLINE) ;
            break;
        case FType::T_S:
            oFlags.set(FOutFlags::F_PLUS, specIt->prec() != 0);
            break;
        case FType::T_COMA:
            oFlags.set(FOutFlags::F_COMA, specIt->prec() != 0);
            break;
        }
        ++specIt;
    }
}

} // namespace FortranFormat

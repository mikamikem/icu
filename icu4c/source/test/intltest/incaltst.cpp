/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Test Internationalized Calendars for C++ */

#include "unicode/utypes.h"
#include "string.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_FORMATTING

#include <stdio.h>

#define CHECK(status, msg) \
    if (U_FAILURE(status)) { \
      errln((UnicodeString(u_errorName(status)) + UnicodeString(" : " ) )+ msg); \
        return; \
    }


static UnicodeString escape( const UnicodeString&src)
{
  UnicodeString dst;
    dst.remove();
    for (int32_t i = 0; i < src.length(); ++i) {
        UChar c = src[i];
        if(c < 0x0080) 
            dst += c;
        else {
            dst += UnicodeString("[");
            char buf [8];
            sprintf(buf, "%#x", c);
            dst += UnicodeString(buf);
            dst += UnicodeString("]");
        }
    }

    return dst;
}


#include "incaltst.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/simpletz.h"
 
// *****************************************************************************
// class IntlCalendarTest
// *****************************************************************************

static UnicodeString fieldName(UCalendarDateFields f);

// Turn this on to dump the calendar fields 
#define U_DEBUG_DUMPCALS  

static UnicodeString calToStr(const Calendar & cal)
{

  UnicodeString out;
  UErrorCode status;
  int i;
  for(i = 0;i<UCAL_FIELD_COUNT;i++) {
    out += (UnicodeString("+") + fieldName((UCalendarDateFields)i) + "=" +  cal.get((UCalendarDateFields)i, status) + UnicodeString(", "));
  }
  out += UnicodeString(cal.getType());

  out += cal.inDaylightTime(status)?UnicodeString("- DAYLIGHT"):UnicodeString("- NORMAL");

  UnicodeString str2;
  out += cal.getTimeZone().getDisplayName(str2);

  return out;
}

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break


void IntlCalendarTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite IntlCalendarTest");
    switch (index) {
    CASE(0,TestTypes);
    CASE(1,TestGregorian);
    CASE(2,TestBuddhist);
    CASE(3,TestJapanese);
    CASE(4,TestBuddhistFormat);
    CASE(5,TestJapaneseFormat);
    default: name = ""; break;
    }
}

#undef CASE

// ---------------------------------------------------------------------------------

static UnicodeString fieldName(UCalendarDateFields f) {
    switch (f) {
    case UCAL_ERA:           return "ERA";
    case UCAL_YEAR:          return "YEAR";
    case UCAL_MONTH:         return "MONTH";
    case UCAL_WEEK_OF_YEAR:  return "WEEK_OF_YEAR";
    case UCAL_WEEK_OF_MONTH: return "WEEK_OF_MONTH";
    case UCAL_DATE:			 return "DAY_OF_MONTH"; // DATE is synonym for DAY_OF_MONTH
    case UCAL_DAY_OF_YEAR:   return "DAY_OF_YEAR";
    case UCAL_DAY_OF_WEEK:   return "DAY_OF_WEEK";
    case UCAL_DAY_OF_WEEK_IN_MONTH: return "DAY_OF_WEEK_IN_MONTH";
    case UCAL_AM_PM:         return "AM_PM";
    case UCAL_HOUR:          return "HOUR";
    case UCAL_HOUR_OF_DAY:   return "HOUR_OF_DAY";
    case UCAL_MINUTE:        return "MINUTE";
    case UCAL_SECOND:        return "SECOND";
    case UCAL_MILLISECOND:   return "MILLISECOND";
    case UCAL_ZONE_OFFSET:   return "ZONE_OFFSET";
    case UCAL_DST_OFFSET:    return "DST_OFFSET";
    case UCAL_YEAR_WOY:      return "YEAR_WOY";
    case UCAL_DOW_LOCAL:     return "DOW_LOCAL";
    case UCAL_FIELD_COUNT:   return "FIELD_COUNT";
    default:
        return UnicodeString("") + ((int32_t)f);
    }
}

/**
 * Test various API methods for API completeness.
 */
void
IntlCalendarTest::TestTypes()
{
  Calendar *c = NULL;
  UErrorCode status = U_ZERO_ERROR;
  int j;
  const char *locs [40] = { "en_US_VALLEYGIRL",     
                            "ja_JP_TRADITIONAL",   
                            "th_TH_TRADITIONAL", 
                            "en_US", NULL };
  const char *types[40] = { "gregorian", 
                            "japanese",
                            "buddhist",           
                            "gregorian", NULL };

  for(j=0;locs[j];j++) {
    logln(UnicodeString("Creating calendar of locale ")  + locs[j]);
    status = U_ZERO_ERROR;
    c = Calendar::createInstance(locs[j], status);
    CHECK(status, "creating '" + UnicodeString(locs[j]) + "' calendar");
    if(U_SUCCESS(status)) {
      logln(UnicodeString(" type is ") + c->getType());
      if(strcmp(c->getType(), types[j])) {
        errln(UnicodeString(locs[j]) + UnicodeString("Calendar type ") + c->getType() + " instead of " + types[j]);
      }
    }
    delete c;
  }
}



/**
 * Run a test of a quasi-Gregorian calendar.  This is a calendar
 * that behaves like a Gregorian but has different year/era mappings.
 * The int[] data array should have the format:
 * 
 * { era, year, gregorianYear, month, dayOfMonth, ...  ... , -1 }
 */
void IntlCalendarTest::quasiGregorianTest(Calendar& cal, const Locale& gcl, const int32_t *data) {
  UErrorCode status = U_ZERO_ERROR;
  // As of JDK 1.4.1_01, using the Sun JDK GregorianCalendar as
  // a reference throws us off by one hour.  This is most likely
  // due to the JDK 1.4 incorporation of historical time zones.
  //java.util.Calendar grego = java.util.Calendar.getInstance();
  Calendar *grego = Calendar::createInstance(gcl, status);

  int32_t tz1 = cal.get(UCAL_ZONE_OFFSET,status);
  int32_t tz2 = grego -> get (UCAL_ZONE_OFFSET, status);
  if(tz1 != tz2) { 
    errln((UnicodeString)"cal's tz " + tz1 + " != grego's tz " + tz2);
  }

  for (int32_t i=0; data[i]!=-1; ) {
    int32_t era = data[i++];
    int32_t year = data[i++];
    int32_t gregorianYear = data[i++];
    int32_t month = data[i++];
    int32_t dayOfMonth = data[i++];
    
    grego->clear();
    grego->set(gregorianYear, month, dayOfMonth);
    UDate D = grego->getTime(status);
    
    cal.clear();
    cal.set(UCAL_ERA, era);
    cal.set(year, month, dayOfMonth);
    UDate d = cal.getTime(status);
#ifdef U_DEBUG_DUMPCALS
    logln((UnicodeString)"cal  : " + calToStr(cal));
    logln((UnicodeString)"grego: " + calToStr(*grego));
#endif
    if (d == D) {
      logln(UnicodeString("OK: ") + era + ":" + year + "/" + (month+1) + "/" + dayOfMonth +
            " => " + d + " (" + UnicodeString(cal.getType()) + ")");
    } else {
      errln(UnicodeString("Fail: (fields to millis)") + era + ":" + year + "/" + (month+1) + "/" + dayOfMonth +
            " => " + d + ", expected " + D + " (" + UnicodeString(cal.getType()) + "Off by: " + (d-D));
    }
    
    // Now, set the gregorian millis on the other calendar
    cal.clear();
    cal.setTime(D, status);
    int e = cal.get(UCAL_ERA, status);
    int y = cal.get(UCAL_YEAR, status);
#ifdef U_DEBUG_DUMPCALS
    logln((UnicodeString)"cal  : " + calToStr(cal));
    logln((UnicodeString)"grego: " + calToStr(*grego));
#endif
    if (y == year && e == era) {
      logln((UnicodeString)"OK: " + D + " => " + cal.get(UCAL_ERA, status) + ":" +
            cal.get(UCAL_YEAR, status) + "/" +
            (cal.get(UCAL_MONTH, status)+1) + "/" + cal.get(UCAL_DATE, status) +  " (" + UnicodeString(cal.getType()) + ")");
    } else {
      errln((UnicodeString)"Fail: (millis to fields)" + D + " => " + cal.get(UCAL_ERA, status) + ":" +
            cal.get(UCAL_YEAR, status) + "/" +
            (cal.get(UCAL_MONTH, status)+1) + "/" + cal.get(UCAL_DATE, status) +
            ", expected " + era + ":" + year + "/" + (month+1) + "/" +
            dayOfMonth +  " (" + UnicodeString(cal.getType()));
    }
  }
  delete grego;
  CHECK(status, "err during quasiGregorianTest()");
}

// Verify that Gregorian works like Gregorian
void IntlCalendarTest::TestGregorian() { 
    int32_t data[] = { 
        GregorianCalendar::AD, 1868, 1868, UCAL_SEPTEMBER, 8,
        GregorianCalendar::AD, 1868, 1868, UCAL_SEPTEMBER, 9,
        GregorianCalendar::AD, 1869, 1869, UCAL_JUNE, 4,
        GregorianCalendar::AD, 1912, 1912, UCAL_JULY, 29,
        GregorianCalendar::AD, 1912, 1912, UCAL_JULY, 30,
        GregorianCalendar::AD, 1912, 1912, UCAL_AUGUST, 1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("de_DE", status);
    CHECK(status, UnicodeString("Creating de_CH calendar"));
    quasiGregorianTest(*cal,Locale("fr_FR"),data);
    delete cal;
}

/**
 * Verify that BuddhistCalendar shifts years to Buddhist Era but otherwise
 * behaves like GregorianCalendar.
 */
void IntlCalendarTest::TestBuddhist() {
    // BE 2542 == 1999 CE
    int32_t data[] = {
        0,           // B. era
        2542,        // B. year
        1999,        // G. year
        UCAL_JUNE,   // month
        4,           // day

        0,           // B. era
        3,           // B. year
        -540,        // G. year
        UCAL_FEBRUARY, // month
        12,          // day

        0,           // test month calculation:  4795 BE = 4252 AD is a leap year, but 4795 AD is not.
        4795,        // BE
        4252,        // AD
        UCAL_FEBRUARY,
        29,

        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("th_TH_TRADITIONAL", status);
    CHECK(status, UnicodeString("Creating th_TH_TRADITIONAL calendar"));
    quasiGregorianTest(*cal,Locale("th_TH"),data);
    delete cal;
}

/**
 * Verify that JapaneseCalendar shifts years to Japanese Eras but otherwise
 * behaves like GregorianCalendar.
 */
void IntlCalendarTest::TestJapanese() {
    
    /* Sorry.. japancal.h is private! */
#define JapaneseCalendar_MEIJI  232
#define JapaneseCalendar_TAISHO 233
#define JapaneseCalendar_SHOWA  234
#define JapaneseCalendar_HEISEI 235
    
    // BE 2542 == 1999 CE
    int32_t data[] = { 
        //       Jera         Jyr  Gyear   m             d
        JapaneseCalendar_MEIJI, 1, 1868, UCAL_SEPTEMBER, 8,
        JapaneseCalendar_MEIJI, 1, 1868, UCAL_SEPTEMBER, 9,
        JapaneseCalendar_MEIJI, 2, 1869, UCAL_JUNE, 4,
        JapaneseCalendar_MEIJI, 45, 1912, UCAL_JULY, 29,
        JapaneseCalendar_TAISHO, 1, 1912, UCAL_JULY, 30,
        JapaneseCalendar_TAISHO, 1, 1912, UCAL_AUGUST, 1,
        
        // new tests (not in java)
        JapaneseCalendar_SHOWA,     64,   1989,  UCAL_JANUARY, 7,  // Test current era transition (different code path than others)
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_JANUARY, 8,   
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_JANUARY, 9,
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_DECEMBER, 20,
        JapaneseCalendar_HEISEI,  15,  2003,  UCAL_MAY, 22,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("ja_JP_TRADITIONAL", status);
    CHECK(status, UnicodeString("Creating ja_JP_TRADITIONAL calendar"));
    quasiGregorianTest(*cal,Locale("ja_JP"),data);
    delete cal;
}

void IntlCalendarTest::TestBuddhistFormat() {
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("th_TH_TRADITIONAL", status);
    CHECK(status, UnicodeString("Creating th_TH_TRADITIONAL calendar"));
    
    // Test simple parse/format with adopt
    
    // First, a contrived english test..
    UDate aDate = 999932400000.0; 
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale("en_US"), status);
    CHECK(status, "creating date format instance");
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt->format(aDate, str);
        logln(UnicodeString() + "Test Date: " + str);
        str.remove();
        fmt->adoptCalendar(cal);
        cal = NULL;
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Buddhist Calendar: " + escape(str));
        UnicodeString expected("September 8, 2544 BE");
        if(str != expected) {
            errln("Expected " + escape(expected) + " but got " + escape(str));
        }
        UDate otherDate = fmt->parse(expected, status);
        if(otherDate != aDate) { 
            UnicodeString str3;
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + escape(expected) + " - wanted " + aDate + " but got " +  otherDate + ", " + escape(str3));
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }
    delete cal;
    
    CHECK(status, "Error occured testing Buddhist Calendar in English ");
    
    status = U_ZERO_ERROR;
    // Now, try in Thai
    {
        UnicodeString expect = CharsToUnicodeString("\\u0E27\\u0E31\\u0E19\\u0E40\\u0E2A\\u0E32\\u0E23\\u0E4C\\u0E17\\u0E35\\u0E48"
            " 8 \\u0E01\\u0E31\\u0e19\\u0e22\\u0e32\\u0e22\\u0e19 \\u0e1e.\\u0e28. 2544");
        UDate         expectDate = 999932400000.0;
        Locale        loc("th_TH_TRADITIONAL");
        
        simpleTest(loc, expect, expectDate, status);
    }
}


void IntlCalendarTest::TestJapaneseFormat() {
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("ja_JP_TRADITIONAL", status);
    CHECK(status, UnicodeString("Creating ja_JP_TRADITIONAL calendar"));
    
    Calendar *cal2 = cal->clone();
    
    // Test simple parse/format with adopt
    
    UDate aDate = 999932400000.0; 
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yy G"), Locale("en_US"), status);
    CHECK(status, "creating date format instance");
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt->format(aDate, str);
        logln(UnicodeString() + "Test Date: " + str);
        str.remove();
        fmt->adoptCalendar(cal);
        cal = NULL;
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Japanese Calendar: " + str);
        UnicodeString expected("September 8, 13 Heisei");
        if(str != expected) {
            errln("Expected " + expected + " but got " + str);
        }
        UDate otherDate = fmt->parse(expected, status);
        if(otherDate != aDate) { 
            UnicodeString str3;
            ParsePosition pp;
            fmt->parse(expected, *cal2, pp);
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + expected + " - wanted " + aDate + " but got " +  " = " +   otherDate + ", " + str3 + " = " + calToStr(*cal2) );
            
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }
    delete cal;
    delete cal2;
    CHECK(status, "Error occured");
    
    // Now, try in Japanese
    {
        UnicodeString expect = CharsToUnicodeString("\\u5e73\\u621013\\u5e749\\u67088\\u65e5");
        UDate         expectDate = 999932400000.0; // Testing a recent date
        Locale        loc("ja_JP_TRADITIONAL");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);
    }
    {
        UnicodeString expect = CharsToUnicodeString("\\u5b89\\u6c385\\u5e747\\u67084\\u65e5");
        UDate         expectDate = -6106035600000.0;
        Locale        loc("ja_JP_TRADITIONAL");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
    {   // Jitterbug 1869 - this is an ambiguous era. (Showa 64 = Jan 6 1989, but Showa could be 2 other eras) )
        UnicodeString expect = CharsToUnicodeString("\\u662d\\u548c64\\u5e741\\u67086\\u65e5");
        UDate         expectDate = 600076800000.0;
        Locale        loc("ja_JP_TRADITIONAL");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
    {   // This Feb 29th falls on a leap year by gregorian year, but not by Japanese year.
        UnicodeString expect = CharsToUnicodeString("\\u5EB7\\u6B632\\u5e742\\u670829\\u65e5");
        UDate         expectDate =  -16214400000000.0;  // courtesy of date format round trip test
        Locale        loc("ja_JP_TRADITIONAL");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
}

void IntlCalendarTest::simpleTest(const Locale& loc, const UnicodeString& expect, UDate expectDate, UErrorCode& status)
{
    UnicodeString tmp;
    UDate         d;
    DateFormat *fmt0 = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull);

    logln("Try format/parse of " + (UnicodeString)loc.getName());
    DateFormat *fmt2 = DateFormat::createDateInstance(DateFormat::kFull, loc);
    if(fmt2) { 
        fmt2->format(expectDate, tmp);
        logln(escape(tmp) + " ( in locale " + loc.getName() + ")");
        if(tmp != expect) {
            errln(UnicodeString("Failed to format " ) + loc.getName() + " expected " + escape(expect) + " got " + escape(tmp) );
        }

        d = fmt2->parse(expect,status);
        CHECK(status, "Error occured parsing " + UnicodeString(loc.getName()));
        if(d != expectDate) {
            fmt2->format(d,tmp);
            errln(UnicodeString("Failed to parse " ) + escape(expect) + ", " + loc.getName() + " expect " + (double)expectDate + " got " + (double)d  + " " + escape(tmp));
            logln( "wanted " + escape(fmt0->format(expectDate,tmp.remove())) + " but got " + escape(fmt0->format(d,tmp.remove())));
        }
        delete fmt2;
    } else {
        errln((UnicodeString)"Can't create " + loc.getName() + " date instance");
    }
    delete fmt0;
}

#undef CHECK

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof

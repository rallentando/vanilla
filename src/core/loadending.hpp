#ifndef LOADENDING_HPP
#define LOADENDING_HPP

#include "switch.hpp"

#include <QRegularExpression>
#include <QString>
#include <QUrl>

namespace LoadEnding {

    enum WebEngineStatus {
        WebEngineStarted   = 0,
        WebEngineStopped   = 1,
        WebEngineSucceeded = 2,
        WebEngineFailed    = 3,
    };

    enum WebEngineErrorDomain {
        WebEngineNoErrorDomain = 0,
        WebEngineHttpStatusCodeDomain = 7,
    };

    enum EdgeWebErrorStatus {
        EdgeUnknown            = 0,
        EdgeConnectionAborted  = 9,
        EdgeOperationCanceled  = 13,
    };

    enum class Verdict {
        Nothing,
        Clear,
        Report,
    };

    enum QuickStatus {
        QuickStarted   = 0,
        QuickStopped   = 1,
        QuickSucceeded = 2,
        QuickFailed    = 3,
    };

    struct QuickVerdict {
        bool endsLoad;
        bool saysFailure;
    };

    inline QuickVerdict QuickEnding(int status){
        QuickVerdict verdict;
        verdict.endsLoad =
            status == QuickStopped || status == QuickSucceeded || status == QuickFailed;
        verdict.saysFailure = status == QuickFailed;
        return verdict;
    }

    inline Verdict WebEngineEnding(int status, int errorDomain){
        if(status == WebEngineStopped) return Verdict::Clear;
        if(status != WebEngineFailed) return Verdict::Nothing;
        if(errorDomain == WebEngineNoErrorDomain) return Verdict::Nothing;
        return Verdict::Report;
    }

    inline QString WebEngineErrorText(QString text, int errorDomain,
                                      int errorCode, const QUrl &url){
        if(errorDomain != WebEngineHttpStatusCodeDomain) return text;
        text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
        text.replace(QStringLiteral("$1"),
                     errorCode == 404 ? url.toString() : url.host());
        return text;
    }

}

#endif

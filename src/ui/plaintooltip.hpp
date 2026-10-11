#ifndef PLAINTOOLTIP_HPP
#define PLAINTOOLTIP_HPP

#include "switch.hpp"

#include <QString>
#include <QTextDocument>

inline QString PlainToolTip(const QString &text){
    if(!Qt::mightBeRichText(text)) return text;
    return QStringLiteral("<p style=\"white-space:pre\">") + text.toHtmlEscaped() + QStringLiteral("</p>");
}

#endif

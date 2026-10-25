#ifndef BLURBEHIND_HPP
#define BLURBEHIND_HPP

#include "switch.hpp"
#include "theme.hpp"

#include <QByteArray>
#include <QSurface>

class QWidget;

namespace BlurBehind {

bool Enabled();

Theme::Role Surface();

void Apply(QWidget *window);

void Blur(QWidget *window);

enum class Mechanism { None, Accent, SystemBackdrop };
enum class Kind { MainWindow, Card };
Mechanism MechanismFor(Kind kind, QSurface::SurfaceType type, bool layered);

bool Available();

bool MainWindowMayBeTranslucent(int widgetsRhi, int highDpiDownscale, const QByteArray &backend);

bool SurfaceMayBeTranslucent(QSurface::SurfaceType type);

}

#endif

/**
 * @file widgets.cppm
 * @brief C++20 module partition for tapiru widgets.
 *
 * Exports: all builder types and component factories.
 */

export module tapiru:widgets;

export {
#include "tapiru/widgets/builders.h"
#include "tapiru/widgets/textarea.h"
#include "tapiru/widgets/progress.h"
#include "tapiru/widgets/spinner.h"
#include "tapiru/widgets/canvas_widget.h"
#include "tapiru/widgets/gauge.h"
#include "tapiru/widgets/paragraph.h"
#include "tapiru/widgets/component_factories.h"
#include "tapiru/widgets/app_components.h"
#include "tapiru/widgets/classic_app.h"
}

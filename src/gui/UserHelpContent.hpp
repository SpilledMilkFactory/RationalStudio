#pragma once

#include <QString>

namespace rationalstudio {
// Offline quick reference is compiled into every GUI build. Full documentation
// ships as docs/USER_GUIDE.md and in both platform packages.
inline QString userHelpHtml() {
    return QString::fromUtf8(R"HTML(
<h2>Rational Studio — User guide</h2>
<p>Four workspaces: <b>Scientific</b>, <b>Graphing</b>, <b>Exact Fractions</b>, and <b>Advanced Math</b>.</p>
<h3>Scientific</h3>
<p>Type an expression and press <b>Enter</b>; try <code>sin(pi/2)+sqrt(9)</code>.
Use RAD/DEG for trigonometry; scientific values are floating-point approximations.</p>
<h3>Graphing</h3>
<p>Select a graph type in the left panel: Cartesian <code>y=x^2</code>, parametric
<code>x(t)=cos(t)</code> and <code>y(t)=sin(t)</code>, polar <code>r=2cos(theta)</code>,
implicit <code>x^2+y^2=25</code>, or vertical inequality <code>y&gt;=x^2</code>.
Drag to pan; mouse wheel zooms; <b>Home</b> resets. Set finite parameter bounds,
use sliders for named scalar variables, and choose <b>Save</b> (JSON), <b>Open</b>,
<b>PNG</b>, <b>Intersections</b> or <b>Features</b>. Graphing and analysis are sampled
numerical approximations.</p>
<h3>Exact Fractions</h3>
<p>Enter two fractions (such as <code>5/2</code> and <code>7/4</code>), choose
an arithmetic operator, and select <b>Calculate</b>. The reduced fraction is exact;
the decimal display is approximate.</p>
<h3>Advanced Math</h3>
<p>Choose a panel: <b>Complex</b>, <b>Calculus</b>, <b>Matrices</b>, <b>Vectors</b>,
<b>Statistics</b> or <b>Regression</b>. Select an operation, enter values and calculate.
These tools are separate from Scientific expression syntax.</p>
<h3>Syntax and keyboard</h3>
<p>Operators <code>+ - * / % ^ !</code>; constants <code>pi e tau</code>;
functions include <code>sin cos tan sqrt ln log abs min max pow hypot</code>.
Use <b>F1</b> for all keyboard shortcuts, <b>Ctrl+1</b>…<b>Ctrl+4</b> for
workspaces, and <b>Ctrl+F1</b> for this guide.</p>
<h3>Help and limitations</h3>
<p>The full <b>USER_GUIDE.md</b> accompanies Linux and Windows packages under
<code>share/doc/rationalstudio/</code> and is available in the repository's
<code>docs/</code> folder. It includes installation, troubleshooting and limitations.
Windows 125% display scaling can clip some graph controls; the scientific and
numerical engines do not implement symbolic algebra.</p>
)HTML");
}
} // namespace rationalstudio

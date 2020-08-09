#include <algorithm>
#include <stack>
#include <vector>
#include <iostream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

//Graham algorithm based on https://www.tutorialspoint.com/cplusplus-program-to-implement-graham-scan-algorithm-to-find-the-convex-hull

typedef double real;

const real epsilon = 0;//(real)1e-15;
const real PI2 = (real)M_PI / 2;

struct Coordinates {
    real x, y;
};

struct Circle: public Coordinates {
    real r;
};

struct Point: public Coordinates  {
    Circle *circle;
    real angle;
};

Point* point0;

Point* extractSecond(stack<Point*> &s) {
    Point* aux = s.top();
    s.pop();
    Point* res = s.top();
    s.push(aux);
    return res;
}

real dist2(const Coordinates* a, const Coordinates* b) {
    real dx = a -> x - b -> x;
    real dy = a -> y - b -> y;
    return dx * dx + dy * dy;
}

real direction(const Coordinates* a, const Coordinates* b, const Coordinates* c) {
    return (b -> y - a -> y) * (c -> x - b -> x) - (b -> x - a -> x) * (c -> y - b -> y);
}

bool comparator(Point* &point1, Point* &point2) {
    real dir = direction(point0, point1, point2);
    if (dir == 0) {
        return dist2(point0, point2) > dist2(point0, point1);
    }
    return dir < 0;
}

void findConvexHull(vector<Point*> &res, vector<Point*> &points) {
    if (points.empty()) {
        return;
    }

    real minY = points[0] -> y;
    int minI = 0;
    int n = points.size();
    for (int i = 1; i < n; i++) {
        real y = points[i] -> y;
        if (y < minY || (minY == y && points[i] -> x < points[minI] -> x)) {
            minY = points[i] -> y;
            minI = i;
        }
    }

    swap(points[0], points[minI]);
    point0 = points[0];

    if (n > 1) {
        sort(points.begin() + 1, points.end(), comparator);
    }
    stack<Point*> s;

    for (int i = 0; i < n; i++) {
        if (i >= 3) {
            while (s.size() >= 2 && direction(extractSecond(s), s.top(), points[i]) > epsilon) {
                s.pop();
            }
        }

        if (!i || dist2(points[i], points[i - 1]) > epsilon) {
            s.push(points[i]);
        }
    }

    while (!s.empty()) {
        res.push_back(s.top());
        s.pop();
    }
}

void removeInternal(vector<Circle*> &out, vector<Circle*> &in) {
    for (vector<Circle*>::iterator i = in.begin(); i != in.end(); i++) {
        const real r1 = (*i)->r;

        bool enclosed = false;
        bool wasI = false;
        for (vector<Circle*>::iterator j = in.begin(); j != in.end(); j++) {
            if (*i == *j) {
                wasI = true;
                continue;
            }
            const real r2 = (*j)->r;
            real d2 = dist2(*i, *j);
            if (d2 < 0) {
                d2 = 0;
            }
            const real d = sqrt(d2) + r1 - r2;
            if (d < -epsilon || (d <= epsilon && wasI)) {
                enclosed = true;
                break;
            }
        }

        if (!enclosed) {
            out.push_back(*i);
        }
    }
}

void outerTangle(vector<Point*> &points, vector<Circle*> &circles) {
    int size = circles.size();
    for (vector<Circle*>::iterator i = circles.begin(); i != circles.end(); i++) {
        const real x1 = (*i)->x;
        const real y1 = (*i)->y;
        const real r1 = (*i)->r;
        for (int a = 0; a < 600; a++) {
            const real angle = (real) M_PI * a / 300;
            Point* p1 = new Point();
            p1 -> x = x1 + r1 * sin(angle);
            p1 -> y = y1 + r1 * cos(angle);
            p1 -> circle = *i;
            p1 -> angle = angle;
            points.push_back(p1);
        }
        for (vector<Circle*>::iterator j = i + 1; j != circles.end(); j++) {
            const real x2 = (*j)->x;
            const real y2 = (*j)->y;
            const real r2 = (*j)->r;
            const real dx = x2 - x1;
            const real dy = y2 - y1;
            const real dr = r2 - r1;
            const real d2 = dx * dx + dy * dy;
            const real d = d2 > 0 ? sqrt(d2) : 0.0;
            if (d < epsilon || abs(dr) > d + epsilon) {
                continue;
            }
            const real beta1 = dr > d - epsilon ? PI2 : dr < -d + epsilon ? -PI2 : asin(dr / d);
            const real gamma = -atan2(dy, dx);
            for (int sign = -1; sign <= 1; sign += 2) {
                const real beta = beta1 * sign;
                const real alpha = gamma - beta;
                const real sinAlpha = sin(alpha) * sign;
                const real cosAlpha = cos(alpha) * sign;

                Point* p1 = new Point();
                p1 -> x = x1 + r1 * sinAlpha;
                p1 -> y = y1 + r1 * cosAlpha;
                p1 -> circle = *i;
                p1 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
                points.push_back(p1);

                Point* p2 = new Point();
                p2 -> x = x2 + r2 * sinAlpha;
                p2 -> y = y2 + r2 * cosAlpha;
                p2 -> circle = *j;
                p2 -> angle = fmod(sign > 0 ? alpha + (real)M_PI * 2: alpha + (real)M_PI * 3, (real)M_PI * 2);
                points.push_back(p2);
            }
        }
    }
}

real beltLength(const Point *a, const Point *b) {
    if (a -> circle != b -> circle) {
        real d2 = dist2(a, b);
        return d2 > 0 ? sqrt(d2) : 0.0;
    } else if (a -> angle >= b -> angle - epsilon) {
        return a -> circle -> r * (M_PI * 2 + b -> angle - a -> angle);
    } else {
        return a -> circle -> r * (b -> angle - a -> angle);
    }
}

real beltLength(vector<Point*> &points) {
    if (points.empty()) {
        return 0;
    }
    real length = 0;
    const Point* prev = points[points.size() - 1];
    vector<Point*>::iterator it;
    for (it = points.begin(); it != points.end(); it++) {
        const Point* p = *it;
        length += beltLength(prev, p);
        prev = p;
    }
    return length;
}

int main() {
    int t;
    cin >> t;

    for (int i = 0; i < t; i++) {
        int n;
        cin >> n;
        vector<Circle*> circles;

        for (int j = 0; j < n; j++) {
            Circle* c = new Circle();
            cin >> c -> x >> c -> y >> c -> r;
            c -> r = fabs(c -> r);
            circles.push_back(c);
        }

        vector<Circle*> circles2;
        removeInternal(circles2, circles);

        if (circles2.size() == 1) {
            cout << fixed << setprecision(10) << circles2[0]->r * M_PI * 2 << endl;
            continue;
        }

        vector<Point*> points;
        outerTangle(points, circles2);

        vector<Point*> result;
        findConvexHull(result, points);

        cout << fixed << setprecision(10) << beltLength(result) << endl;
    }

    return 0;
}

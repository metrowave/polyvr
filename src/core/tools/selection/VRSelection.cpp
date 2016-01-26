#include "VRSelection.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeometry.h>

/* not compiling?
install liblapacke-dev
sudo apt-get install liblapacke-dev
*/

#include <lapacke.h>
#define dgeev LAPACKE_dgeev_work

using namespace OSG;

VRSelection::VRSelection() { clear(); }

VRSelectionPtr VRSelection::create() { return VRSelectionPtr( new VRSelection() ); }

bool VRSelection::vertSelected(Vec3f p) { return false; }
bool VRSelection::objSelected(VRGeometryPtr geo) { return false; }
bool VRSelection::partialSelected(VRGeometryPtr geo) { return false; }

void VRSelection::add(VRGeometryPtr geo, vector<int> subselection) {
    auto k = geo.get();
    if (selected.count(k) == 0) selected[k] = selection_atom();
    selected[k].geo = geo;
    selected[k].subselection = subselection;
}

void VRSelection::clear() {
    selected.clear();
    bbox.clear();
}

void VRSelection::apply(VRObjectPtr tree, bool force) {
    if (!tree) return;

    vector<VRGeometryPtr> geos;
    for ( auto c : tree->getChildren(true) ) if (c->hasAttachment("geometry")) geos.push_back( static_pointer_cast<VRGeometry>(c) );
    if ( tree->hasAttachment("geometry") ) geos.push_back( static_pointer_cast<VRGeometry>(tree) );

    for (auto geo : geos) {
        selection_atom a;
        a.geo = geo;
        if ( objSelected(geo) || force);
        else if ( partialSelected(geo) ) a.partial = true;
        else continue;
        selected[geo.get()] = a;
    }
}

void VRSelection::append(VRSelectionPtr sel) {
    for (auto& s : sel->selected) {
        if (!selected.count(s.first)) selected[s.first] = s.second;
        else {
            auto& s1 = selected[s.first].subselection;
            auto& s2 = s.second.subselection;
            s1.insert( s1.end(), s2.begin(), s2.end() );
        }
    }
}

vector<VRGeometryWeakPtr> VRSelection::getPartials() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) if (g.second.partial) res.push_back(g.second.geo);
    return res;
}

vector<VRGeometryWeakPtr> VRSelection::getSelected() {
    vector<VRGeometryWeakPtr> res;
    for (auto g : selected) {
        if (!g.second.partial) {
            res.push_back(g.second.geo);
        }
    }
    return res;
}

void VRSelection::updateSubselection() {
    for (auto s : selected) updateSubselection(s.second.geo.lock());
}

void VRSelection::updateSubselection(VRGeometryPtr geo) {
    if (!geo) return;
    if ( !selected.count( geo.get() ) ) {
        selection_atom s;
        s.geo = geo;
        selected[geo.get()] = s;
    }

    auto& sel = selected[geo.get()];
    Matrix m = geo->getWorldMatrix();
    sel.subselection.clear();
    if (!geo->getMesh()) return;
    auto pos = geo->getMesh()->getPositions();
    if (!pos) return;
    for (int i=0; i<pos->size(); i++) {
        Pnt3f p = pos->getValue<Pnt3f>(i);
        m.mult(p,p);
        if (vertSelected(Vec3f(p))) {
            bbox.update(Vec3f(p));
            sel.subselection.push_back(i);
        }
    }
}

vector<int> VRSelection::getSubselection(VRGeometryPtr geo) {
    if (!geo) return vector<int>();
    if ( !selected.count( geo.get() ) ) updateSubselection(geo);
    if ( !selected.count( geo.get() ) ) return vector<int>();
    return selected[geo.get()].subselection;
}

map< VRGeometryPtr, vector<int> > VRSelection::getSubselections() {
    map< VRGeometryPtr, vector<int> > res;
    for (auto s : selected) {
        auto sp = s.second.geo.lock();
        if (sp) res[sp] = s.second.subselection;
    }
    return res;
}

Vec3f VRSelection::computeCentroid() {
    Vec3f res;
    int N = 0;
    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->getPositions();
        N += s.second.subselection.size();
        for (auto i : s.second.subselection) {
            auto p = Vec3f(pos->getValue<Pnt3f>(i));
            res += p;
        }
    }

    if (N > 0) res *= 1.0/N;
    cout << " centroid: " << res << endl;
    return res;
}

Matrix VRSelection::computeCovMatrix() {
    Vec3f center = computeCentroid();
    int N = 0;
    Matrix res(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0); // 3x3?

    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->getPositions();
        N += s.second.subselection.size();
        for (auto i : s.second.subselection) {
            auto pg = Vec3f(pos->getValue<Pnt3f>(i));
            Vec3f p = pg - center;
            res[0][0] += p[0]*p[0];
            res[1][1] += p[1]*p[1];
            res[2][2] += p[2]*p[2];
            res[0][1] += p[0]*p[1];
            res[0][2] += p[0]*p[2];
            res[1][2] += p[1]*p[2];
        }
    }

    for (int i=0; i<3; i++)
        for (int j=i; j<3; j++) res[i][j] *= 1.0/N;

    res[1][0] = res[0][1];
    res[2][0] = res[0][2];
    res[2][1] = res[1][2];

    res[3][0] = center[0];
    res[3][1] = center[1];
    res[3][2] = center[2];

    cout << " covariance matrix: " << res[0][0] << " " << res[1][1] << " " << res[2][2] << " " << res[0][1] << " " << res[0][2] << " " << res[1][2] << endl;
    return res;
}

Matrix VRSelection::computeEigenvectors(Matrix m) {
    int n = 3, lda = 3, ldvl = 3, ldvr = 3, info, lwork;
    double wkopt;
    double* work;
    double wr[3], wi[3], vl[9], vr[9];
    double a[9] = { m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2] };

    //LAPACK_COL_MAJOR LAPACK_ROW_MAJOR

    lwork = -1;
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr, &wkopt, lwork);
    lwork = (int)wkopt;
    work = new double[lwork];
    info = dgeev( LAPACK_COL_MAJOR, 'V', 'V', n, a, lda, wr, wi, vl, ldvl, vr, ldvr,  work, lwork);
    delete work;

    if ( info > 0 ) { cout << "Warning: computeEigenvalues failed!\n"; return Matrix(); } // Check for convergence

    Matrix res;
    res[0] = Vec4f(vl[0], vl[1], vl[2], 0);
    res[1] = Vec4f(vl[3], vl[4], vl[5], 0);
    res[2] = Vec4f(vl[6], vl[7], vl[8], 0);
    res[3] = Vec4f(wr[0], wr[1], wr[2], 0);
    return res;
}

pose VRSelection::computePCA() {
    pose res;
    Matrix cov = computeCovMatrix();
    Matrix ev  = computeEigenvectors(cov);

    Vec3f pos = Vec3f(cov[3]);
    //dir =

    res.set(pos, Vec3f(ev[0]), Vec3f(ev[2]));

    return res;
}

void VRSelection::selectPlane(pose p, float threshold) {
    Plane plane( p.up(), Pnt3f(p.pos()) );

    for (auto& s : selected) {
        auto geo = s.second.geo.lock();
        auto pos = geo->getMesh()->getPositions();
        s.second.subselection.clear();
        for (int i=0; i<pos->size(); i++) {
            auto p = pos->getValue<Pnt3f>(i);
            auto d = plane.distance(p);
            if (abs(d) < threshold) s.second.subselection.push_back(i);
        }
    }
}





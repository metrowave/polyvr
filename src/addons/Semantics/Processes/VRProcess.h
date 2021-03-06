#ifndef VRProcess_H_INCLUDED
#define VRProcess_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/utils/VRName.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"

#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

enum PROCESS_WIDGET {
    SUBJECT,
    MESSAGE,
    STATE,
    TRANSITION
};

enum TRANSITION_CONDITION {
    SEND_CONDITION,
    RECEIVE_CONDITION,
    NONE
};

struct VRProcessNode : VRName {
    VREntityPtr entity;
    VRTransformPtr widget;
    PROCESS_WIDGET type;
    TRANSITION_CONDITION transition = NONE;
    string label;
    int ID = 0;
    int subject = 0;
    bool isInitialState = false;
    bool isSendState = false;
    bool isReceiveState = false;
    //VRProcessNodePtr message;

    VRProcessNode(string name, PROCESS_WIDGET type, int ID, int sID);
    ~VRProcessNode();
    static VRProcessNodePtr create(string name, PROCESS_WIDGET type, int ID, int sID);

    void update(Graph::node& n, bool changed);

    int getID();
    string getLabel();
    VREntityPtr getEntity();
    int getSubjectID();
    Vec3d getPosition(Vec3d p, float scale = 1);
};

struct VRProcessDiagram : public Graph {
    map<int, VRProcessNodePtr> processnodes;

    VRProcessDiagram();
    ~VRProcessDiagram();
    static VRProcessDiagramPtr create();

    void update(int i, bool changed);
    void remNode(int i);
    vector<VRProcessNodePtr> getNodes();
    void clear();
};

class VRProcess : public std::enable_shared_from_this<VRProcess>, public VRName {
    private:
        VROntologyPtr ontology;
        VRProcessDiagramPtr interactionDiagram;
        map<int, VRProcessDiagramPtr> behaviorDiagrams;
        void printNodes(VRProcessDiagramPtr d);
        map<VRProcessNodePtr, VRProcessNodePtr> stateToMessage; //maps state to message for send/receive refenrences
        map<VRProcessNodePtr, VRProcessNodePtr> transitionToMessage; //maps the send/receive transition node to the corresponding message node

        void update();

    public:
        VRProcess(string name);
        static VRProcessPtr create(string name);
        VRProcessPtr ptr();

        void open(string path);
        void setOntology(VROntologyPtr o);
        VROntologyPtr getOntology();
        VRProcessDiagramPtr getInteractionDiagram();
        VRProcessDiagramPtr getBehaviorDiagram(int subject);
        VRProcessNodePtr getNode(int i, VRProcessDiagramPtr diag = 0);

        bool isFunctionState(VRProcessNodePtr); // = neither send/receive state
        bool isSendState(VRProcessNodePtr);
        bool isReceiveState(VRProcessNodePtr);
        bool isInitialState(VRProcessNodePtr);

        vector<VRProcessNodePtr> getSubjectMessages(int subjectID);
        vector<VRProcessNodePtr> getOutgoingMessages(int subjectID);
        vector<VRProcessNodePtr> getIncomingMessages(int subjectID);
        vector<VRProcessNodePtr> getMessageSubjects(int messageID);
        vector<VRProcessNodePtr> getMessageReceiver(int messageID);
        vector<VRProcessNodePtr> getMessageSender(int messageID);
        vector<VRProcessNodePtr> getSubjects();
        vector<VRProcessNodePtr> getMessages();
        vector<VRProcessNodePtr> getSubjectStates(int subjectID);
        vector<VRProcessNodePtr> getSubjectTransitions(int subjectID); // all subject edges
        vector<VRProcessNodePtr> getStateTransitions(int subjectID, int stateID); // all state edges
        vector<VRProcessNodePtr> getStateOutTransitions(int subjectID, int stateID); //only outgoing edges
        vector<VRProcessNodePtr> getTransitionStates(int subjectID, int transitionID);
        vector<VRProcessNodePtr> getTransitionSourceState(int subjectID, int transitionID);
        vector<VRProcessNodePtr> getTransitions(int subjectID);
        vector<VRProcessNodePtr> getInitialStates();

        VRProcessNodePtr getStateMessage(VRProcessNodePtr state);
        VRProcessNodePtr getTransitionMessage(VRProcessNodePtr transition);

        VRProcessNodePtr addSubject(string name, VREntityPtr e = 0);
        VRProcessNodePtr addMessage(string name, int i, int j, VRProcessDiagramPtr diag = 0, VREntityPtr e = 0);
        VRProcessNodePtr addState(string name, int sID, VREntityPtr e = 0);
        VRProcessNodePtr addTransition(string name, int sID, int i, int j, VRProcessDiagramPtr d = 0);
        void setInitialState(VRProcessNodePtr state);

        void remNode(VRProcessNodePtr n);
        VRProcessNodePtr getTransitionState(VRProcessNodePtr transition);
};

OSG_END_NAMESPACE;

#endif // VRProcess_H_INCLUDED

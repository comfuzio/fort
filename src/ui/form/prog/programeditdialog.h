#ifndef PROGRAMEDITDIALOG_H
#define PROGRAMEDITDIALOG_H

#include <form/controls/formwindow.h>
#include <model/applistmodel.h>

class ConfAppManager;
class ConfRuleManager;
class ConfManager;
class FirewallConf;
class FortManager;
class IniUser;
class LineEdit;
class PlainTextEdit;
class ProgramsController;
class SpinCombo;
class WindowManager;
class ZonesSelector;

class ProgramEditDialog : public FormWindow
{
    Q_OBJECT

public:
    explicit ProgramEditDialog(
            ProgramsController *ctrl, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    ProgramsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    ConfAppManager *confAppManager() const;
    ConfRuleManager *confRuleManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    AppListModel *appListModel() const;

    bool isWildcard() const { return m_isWildcard; }
    bool isNew() const { return m_appRow.appId == 0; }

    void initialize(const AppRow &appRow, const QVector<qint64> &appIdList = {});

protected:
    virtual void closeOnSave();

    void setAdvancedMode(bool on);

    quint16 currentRuleId() const { return m_currentRuleId; }
    void setCurrentRuleId(quint16 ruleId = 0) { m_currentRuleId = ruleId; }

private:
    enum ApplyChildType : qint8 {
        Disabled = 0,
        ToSpecChild,
        ToChild,
        FromParent,
    };

    void initializePathNameRuleFields(bool isSingleSelection = true);
    void initializePathField(bool isSingleSelection);
    void initializeNameField(bool isSingleSelection);
    void initializeRuleField(bool isSingleSelection);
    void initializeFocus();

    QIcon appIcon(bool isSingleSelection) const;

    void setupController();
    void setupRuleManager();

    void retranslateUi();
    void retranslatePathPlaceholderText();
    void retranslateComboApplyChild();
    void retranslateScheduleAction();
    void retranslateScheduleType();
    void retranslateScheduleIn();
    virtual void retranslateWindowTitle();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupPathLayout();
    QLayout *setupNameLayout();
    QLayout *setupApplyChildGroupLayout();
    void setupComboAppGroups();
    QLayout *setupActionsLayout();
    void setupActionsGroup();
    QLayout *setupZonesRuleLayout();
    QLayout *setupRuleLayout();
    QLayout *setupScheduleLayout();
    void setupCbSchedule();
    void setupComboScheduleType();
    QLayout *setupButtonsLayout();
    void setupOptions();
    void setupChildOptionsLayout();
    void setupLogOptions();
    void setupSwitchWildcard();

    void updateZonesRulesLayout();
    void updateApplyChild();
    void updateWildcard(bool isSingleSelection = true);

    void switchWildcardPaths();

    void fillEditName();

    bool save();
    bool saveApp(App &app);
    bool saveMulti(App &app);

    bool validateFields() const;
    void fillApp(App &app) const;
    void fillAppPath(App &app) const;
    void fillAppApplyChild(App &app) const;
    void fillAppEndTime(App &app) const;

    QString getEditText() const;

    void selectRuleDialog();
    void editRuleDialog(int ruleId);

    void warnDangerousOption() const;
    void warnRestartNeededOption() const;

private:
    bool m_isWildcard = false;

    quint16 m_currentRuleId = 0;

    ProgramsController *m_ctrl = nullptr;

    QLabel *m_labelEditPath = nullptr;
    LineEdit *m_editPath = nullptr;
    PlainTextEdit *m_editWildcard = nullptr;
    QToolButton *m_btSelectFile = nullptr;
    QLabel *m_labelEditName = nullptr;
    LineEdit *m_editName = nullptr;
    QToolButton *m_btGetName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QLabel *m_labelApplyChild = nullptr;
    QComboBox *m_comboApplyChild = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QRadioButton *m_rbKillProcess = nullptr;
    QButtonGroup *m_btgActions = nullptr;

    QCheckBox *m_cbKillChild = nullptr;
    QCheckBox *m_cbParked = nullptr;
    QCheckBox *m_cbLogAllowedConn = nullptr;
    QCheckBox *m_cbLogBlockedConn = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    LineEdit *m_editRuleName = nullptr;
    QToolButton *m_btSelectRule = nullptr;

    QCheckBox *m_cbSchedule = nullptr;
    QComboBox *m_comboScheduleAction = nullptr;
    QComboBox *m_comboScheduleType = nullptr;
    SpinCombo *m_scScheduleIn = nullptr;
    QDateTimeEdit *m_dteScheduleAt = nullptr;

    QPushButton *m_btOptions = nullptr;
    QToolButton *m_btSwitchWildcard = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;
    QPushButton *m_btMenu = nullptr;

    AppRow m_appRow;
    QVector<qint64> m_appIdList;
};

#endif // PROGRAMEDITDIALOG_H

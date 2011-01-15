#ifndef RSIPAUTHENTICATION_H_INCLUDED
#define RSIPAUTHENTICATION_H_INCLUDED
#include <qdialog.h>
//#include <q3listview.h>
#include <qcheckbox.h>
//#include <q3button.h>
#include <qlabel.h>

#include "sipcall.h"
#include "ridentityform.h"


// Диалог с запросом имени пользователя и пароля для аутентификации
class KSipAuthenticationRequest : public QDialog
{
  Q_OBJECT
public:
  KSipAuthenticationRequest( const QString &server, const QString &sipuri,
    const QString &prefix, const QString &authtype, 
    QWidget *parent = 0, const char *name = 0 );
  ~KSipAuthenticationRequest( void );

  void setUsername( const QString &newUsername );
  void setPassword( const QString &newPassword );
  QString getUsername( void );
  QString getPassword( void );

private slots:
  void okClicked( void );

private:
  QCheckBox *savePassword;
  QLineEdit *username;
  QLineEdit *password;
  QPushButton *okPushButton;
  QPushButton *cancelPushButton;
  QString userPrefix;
};



// Класс аутентификации
class KSipAuthentication : public QObject
{
  Q_OBJECT
public:
  KSipAuthentication( void );
  ~KSipAuthentication( void );

public slots:
  void authRequest( SipCallMember *member );

private:
  KSipAuthenticationRequest *_authRequest;
  bool _execAuthreq;


//private:
  //void setRegisterState( void );
  //void save( void );
  //SipClient *c;

  ////KSipIdentityEdit *edit;
  ////KPhoneView *view;
  //QString userPrefix;
  //SipUser *u;
  //SipRegister *sipreg;
  //bool autoRegister;
  //int expires;
  //QWidget *parent;
  //bool useStun;
  //QString stunSrv;
};

#endif // KSIPAUTHENTICATION_H_INCLUDED

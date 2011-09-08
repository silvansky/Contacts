#include <stdio.h>
//#include <q3hbox.h>
//#include <q3vbox.h>
#include <qlabel.h>
//#include <q3frame.h>
#include <qlineedit.h>
//#include <q3hbuttongroup>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qsettings.h>
#include <qlayout.h>
//Added by qt3to4:
//#include <Q3HBoxLayout>
//#include <Q3VBoxLayout>

#include "sipcall.h"
#include "sipuser.h"
#include "sipregister.h"
#include "sipclient.h"
//#include "kphone.h"
//#include "kphoneview.h"
#include "rsipauthentication.h"


KSipAuthenticationRequest::KSipAuthenticationRequest(
  const QString &server, const QString &sipuri, const QString &prefix,
  const QString &authtype, QWidget *parent, const char *name )
  : QDialog( parent )
{
	Q_UNUSED(name);
	Q_UNUSED(authtype);
  //setName(prefix + authtype);

  userPrefix = prefix;
  //Q3VBox *vbox = new Q3VBox( this );
  //vbox->setMargin( 3 );
  //vbox->setSpacing( 3 );
  QVBoxLayout *vboxl = new QVBoxLayout( this );
  //vboxl->addWidget( vbox );
  vboxl->addWidget( new QLabel( tr("SipUri = ") + sipuri, this) );
  vboxl->addWidget( new QLabel( tr("Server = ") + server, this) );
  vboxl->addWidget( new QLabel( tr("Username:"), this) );
  username = new QLineEdit( this );
  username->setAttribute(Qt::WA_MacShowFocusRect, false);
  vboxl->addWidget( username );
  vboxl->addWidget( new QLabel( tr("Password:"), this) );
  password = new QLineEdit( this );
  password->setAttribute(Qt::WA_MacShowFocusRect, false);
  password->setEchoMode( QLineEdit::Password );
  password->setFocus();
  vboxl->addWidget( password );
  savePassword = new QCheckBox( tr("Save password"), this );
  vboxl->addWidget(savePassword);
  QHBoxLayout *buttonBox;
  buttonBox = new QHBoxLayout( this );
  vboxl->addItem(buttonBox);
  okPushButton = new QPushButton( tr("ok button"), this );
  okPushButton->setText( tr("OK") );
  okPushButton->setDefault( TRUE );
  buttonBox->addWidget( okPushButton );
  cancelPushButton = new QPushButton( tr("cancel button"), this );
  cancelPushButton->setText( tr("Cancel") );
  //cancelPushButton->setAccel( Qt::Key_Escape );
  buttonBox->addWidget( cancelPushButton );
  connect( okPushButton, SIGNAL( clicked() ), this, SLOT( okClicked() ) );
  connect( cancelPushButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

KSipAuthenticationRequest::~KSipAuthenticationRequest( void )
{
}

void KSipAuthenticationRequest::setUsername( const QString &newUsername )
{
  username->setText( newUsername );
}

void KSipAuthenticationRequest::setPassword( const QString &newPassword )
{
  password->setText( newPassword );
}

QString KSipAuthenticationRequest::getUsername( void )
{
  return username->text();
}

QString KSipAuthenticationRequest::getPassword( void )
{
  return password->text();
}

void KSipAuthenticationRequest::okClicked( void )
{
  if (savePassword->isChecked())
  {
	QString p = "/settings/Registration/";
	QSettings().setValue( p + "Password", getPassword() );
  }
  accept();
}






KSipAuthentication::KSipAuthentication()
{
  _authRequest = NULL;
  _execAuthreq = false;
}

KSipAuthentication::~KSipAuthentication( void )
{
  if(_authRequest == NULL)
  {
	delete _authRequest;
	_authRequest = NULL;
  }
}

void KSipAuthentication::authRequest( SipCallMember *member )
{
  if( member->getAuthState() != SipCallMember::authState_AuthenticationRequired &&
	  member->getAuthState() != SipCallMember::authState_AuthenticationRequiredWithNewPassword )
  {
	return;
  }

  QString userName = member->getCall()->getProxyUsername();
  QString password = member->getCall()->getPassword();

  if( _execAuthreq )
	return;

  // Если не задан пароль или требуется аутентификация по новому паролю
  if( password.isEmpty() || member->getAuthState() == SipCallMember::authState_AuthenticationRequiredWithNewPassword )
  {
	QString proxy = member->getCall()->getSipProxy();
	SipUri localURI = member->getCall()->localAddress();

	if( !_authRequest )
	{
	  _authRequest = new KSipAuthenticationRequest( proxy, localURI.uri(), QString::null, QString::null );
	}

	_authRequest->setUsername( userName );
	_authRequest->setPassword( password );

	// Вызов диалога запроса пароля
	_execAuthreq = true;
	if( _authRequest->exec() )
	{
	  userName = _authRequest->getUsername();
	  password = _authRequest->getPassword();
	  if( userName.isEmpty() || password.isEmpty() )
	  {
	return;
	  }
	  member->getCall()->setPassword( password );
	  _execAuthreq = false;
	}
	else
	{
	  _execAuthreq = false;
	  return;
	}
  }

  switch( member->getCallMemberType() )
  {
  case SipCallMember::Subscribe:
	member->sendRequestSubscribe( userName, password );
	break;
  case SipCallMember::Notify:
	member->sendRequestNotify( userName, password );
	break;
  case SipCallMember::Message:
	member->sendRequestMessage( userName, password );
	break;
  case SipCallMember::Invite:
	member->sendRequestInvite( userName, password );
	break;
  default:
	break;
  }
}

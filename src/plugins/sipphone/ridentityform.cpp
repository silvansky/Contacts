#include "ridentityform.h"

#include <QSettings>
#include <QMessageBox>

#include <SipClient.h>
#include <SipUser.h>
#include <SipRegister.h>
#include "SipPhoneProxy.h"
#include "rsipauthentication.h"

RIdentityForm::RIdentityForm(QWidget *parent, const char *name, QObject *reg) : QDialog(parent)
{
  ui.setupUi(this);

  connect( ui.buttonRegister, SIGNAL( clicked() ), reg, SLOT( changeRegistration() ) );


}

RIdentityForm::~RIdentityForm()
{

}

void RIdentityForm::updateState( RSipState state )
{
  if( state == OFFLINE )
  {
    ui.buttonRegister->setText( tr("Register : not registered") );
    ui.buttonRegister->setEnabled( false );
  }
  else if( state == REG )
  {
    ui.labelRegister->setText( tr("Registration : registered"));
    ui.buttonRegister->setText( tr("Unregister") );
    ui.buttonRegister->setEnabled( true );
  }
  else if ( state == UNREG )
  {
    ui.labelRegister->setText( tr("Registration : not registered"));
    ui.buttonRegister->setText( tr("Register") );
    ui.buttonRegister->setEnabled( true );
  }
  else
  {
    ui.labelRegister->setText( tr("Registration : ") );
  }
}

QString RIdentityForm::getFullname( void ) const
{
  return ui.fullname->text();
}

QString RIdentityForm::getUsername( void ) const
{
  return ui.username->text();
}

QString RIdentityForm::getHostname( void ) const
{
  return ui.hostname->text();
}

QString RIdentityForm::getSipProxy( void ) const
{
  return ui.sipProxy->text();
}

QString RIdentityForm::getSipProxyUsername( void ) const
{
  return ui.sipProxyUsername->text();
}


QString RIdentityForm::getUri( void ) const
{
  return "\"" + ui.fullname->text() + "\" <sip:" + ui.username->text() + "@" + ui.hostname->text() + ">";
}

void RIdentityForm::setFullname( const QString &newFullname )
{
  ui.fullname->setText( newFullname );
}

void RIdentityForm::setUsername( const QString &newUsername )
{
  ui.username->setText( newUsername );
}

void RIdentityForm::setHostname( const QString &newHostname )
{
  ui.hostname->setText( newHostname );
}

void RIdentityForm::setSipProxy( const QString &newSipProxy )
{
  ui.sipProxy->setText( newSipProxy );
}

void RIdentityForm::setSipProxyUsername( const QString &newSipProxyUsername )
{
  ui.sipProxyUsername->setText( newSipProxyUsername );
}

void RIdentityForm::accept()
{
  if( ui.username->text() == "" || ui.hostname->text() == "" )
  {
    QDialog::reject();
  }
  else
  {
    emit RIdentityForm::updateForm();
    QDialog::accept();
  }
}
void RIdentityForm::reject()
{
  QDialog::reject();
}
////////////////////////////////////////////////////////////////////////////////






RSipRegistrations::RSipRegistrations(const SipRegistrationData& regData, SipClient *sipClient, SipPhoneProxy *phoneObject, QWidget *parentWidget, const char *prefix )
{
  _parentWidget = parentWidget;
  _extSipClient = sipClient;
  _extPhoneObject = phoneObject;
  _userPrefix = prefix;
  _useStun = false;

  _pIdentityForm = new RIdentityForm( _parentWidget, _userPrefix.toLatin1(), this );
  connect( _pIdentityForm, SIGNAL( updateForm() ), this, SLOT( update() ) );

  _sipRegister = 0;

  //QString uristr;
  //SipUri uri;
  //QSettings settings;
  //QString pp = "/settings/local/";

  _expires = regData.registrationExpiresTime;// settings.value( pp + "/RegistrationExpiresTime", constRegistrationExpiresTime ).toInt();
  if( _expires == 0 )
  {
    _expires = -1;
  }

  //pp = "/settings/Registration/";
  if( !regData.sipUserUri.isEmpty() )// settings.value( pp + "/SipUri", "" ).toString() != "" )
  {
    QString uristr = regData.sipUserUri; //settings.value( pp + "/SipUri" ).toString();
    SipUri uri = SipUri( uristr );
    _pSipUser = _extSipClient->getUser( uri );
    if( _pSipUser == NULL )
    {
      _pSipUser = new SipUser( _extSipClient, uri );
    }

    _stunServer = "";
    if( regData.useStun )//  settings.value( "/settings/STUN/UseStun", "" ).toString() == "Yes" )
    {
      _useStun = regData.useStun; //true;
      //_stunServer = settings.value( "/settings/STUN/StunServer", constStunServer ).toString();
      _stunServer = regData.stunServerWithPort;

      if( _stunServer.isEmpty() )
      {
        QString dname = _pSipUser->getMyUri()->getHostname();
        _stunServer = dname;
        QString srv = sipClient->getSRV( QString( "_stun._udp." ) + dname );
        if( !srv.isEmpty() )
        {
          _stunServer = srv;
        }
        _stunServer += ":3478";
      }
      else
      {
        if( !_stunServer.contains( ':' ) )
        {
          _stunServer += ":3478";
        }
      }
    }

    QString sipServerUri = regData.sipServerUri;
    // uristr = "";
    //if( settings.value( pp + "/SipServer", "" ).toString() != "" )
    //{
    //  uristr = settings.value( pp + "/SipServer" ).toString();
    //}
    //QString qvalue = settings.value( pp + "/qValue", "" ).toString();
    QString qValue = regData.qValue;

    //_sipReg = new SipRegister( _sipUser, SipUri( uristr ), _expires, qvalue );
    _sipRegister = new SipRegister( _pSipUser, SipUri( sipServerUri ), _expires, qValue );
    connect( _sipRegister, SIGNAL( statusUpdated() ), this, SLOT( registerStatusUpdated() ) );
    connect( _sipRegister, SIGNAL(trueRegistrationStatus(bool)), this, SIGNAL(proxyTrueRegistrationStatus(bool)));
		
		// ÎÒËÀÄÊÀ
		//connect( _sipRegister, SIGNAL(trueRegistrationStatus(bool)), this, SLOT(trueRegistrationStatusSlot(bool)));


    _extPhoneObject->updateIdentity( _pSipUser, _sipRegister );
    _extSipClient->updateIdentity( _pSipUser, _sipRegister->getOutboundProxy() );

		if(!regData.userName.isEmpty())
			_pSipUser->getMyUri()->setProxyUsername( regData.userName );
		if(!regData.password.isEmpty())
			_pSipUser->getMyUri()->setPassword( regData.password );
    
		_autoRegister = regData.autoRegister;
    if( _autoRegister )
    {
      if( _useStun )
      {
        _sipRegister->setAutoRegister( _autoRegister );
      }
      else
      {
        _sipRegister->requestRegister();
      }
    }
    else
    {
      _sipRegister->setAutoRegister( _autoRegister );
    }

    
    

    //////////QString str;
    //////////str = settings.value( pp + "/UserName" ).toString();
    //////////_sipUser->getMyUri()->setProxyUsername( str );
    //////////str = settings.value( pp + "/Password" ).toString();
    //////////_sipUser->getMyUri()->setPassword( str );
    //////////str = settings.value( pp + "/AutoRegister" ).toString();
    //////////if( str == "Yes" )
    //////////{
    //////////  _autoRegister = true;
    //////////  if( _useStun )
    //////////  {
    //////////    _sipReg->setAutoRegister( true );
    //////////  }
    //////////  else
    //////////  {
    //////////    _sipReg->requestRegister();
    //////////  }
    //////////}
    //////////else
    //////////{
    //////////  _autoRegister = false;
    //////////  _sipReg->setAutoRegister( false );
    //////////}
  }
  else
  {
    editRegistration();
  }
}

void RSipRegistrations::trueRegistrationStatusSlot(bool state)
{
	QMessageBox::information(NULL, "debug", "RSipRegistrations::trueRegistrationStatusSlot");
}


//////////RSipRegistrations::RSipRegistrations( SipClient *sipClient, RamblerProto *phoneObject, QWidget *parentWidget, const char *prefix )
//////////{
//////////  _parentWidget = parentWidget;
//////////  _sipClient = sipClient;
//////////  _phoneObject = phoneObject;
//////////  _userPrefix = prefix;
//////////  _useStun = false;
//////////
//////////  _identityForm = new RIdentityForm( _parentWidget, _userPrefix.toLatin1(), this );
//////////  connect( _identityForm, SIGNAL( update() ), this, SLOT( update() ) );
//////////
//////////  _sipReg = 0;
//////////  QString uristr;
//////////  QString str;
//////////  SipUri uri;
//////////  QSettings settings;
//////////  QString pp = "/settings/local/";
//////////
//////////  _expires = settings.value( pp + "/RegistrationExpiresTime", constRegistrationExpiresTime ).toInt();
//////////
//////////  if( _expires == 0 )
//////////  {
//////////    _expires = -1;
//////////  }
//////////  pp = "/settings/Registration/";
//////////  if( settings.value( pp + "/SipUri", "" ).toString() != "" )
//////////  {
//////////    uristr = settings.value( pp + "/SipUri" ).toString();
//////////    uri = SipUri( uristr );
//////////    _sipUser = _sipClient->getUser( uri );
//////////    if( _sipUser == NULL )
//////////    {
//////////      _sipUser = new SipUser( _sipClient, uri );
//////////    }
//////////    _stunServer = "";
//////////    if( settings.value( "/settings/STUN/UseStun", "" ).toString() == "Yes" )
//////////    {
//////////      _useStun = true;
//////////      _stunServer = settings.value( "/settings/STUN/StunServer", constStunServer ).toString();
//////////      if( _stunServer.isEmpty() )
//////////      {
//////////        QString dname = _sipUser->getMyUri()->getHostname();
//////////        _stunServer = dname;
//////////        QString srv = sipClient->getSRV( QString( "_stun._udp." ) + dname );
//////////        if( !srv.isEmpty() )
//////////        {
//////////          _stunServer = srv;
//////////        }
//////////        _stunServer += ":3478";
//////////      }
//////////      else
//////////      {
//////////        if( !_stunServer.contains( ':' ) )
//////////        {
//////////          _stunServer += ":3478";
//////////        }
//////////      }
//////////    }
//////////    uristr = "";
//////////    if( settings.value( pp + "/SipServer", "" ).toString() != "" )
//////////    {
//////////      uristr = settings.value( pp + "/SipServer" ).toString();
//////////    }
//////////    QString qvalue = settings.value( pp + "/qValue", "" ).toString();
//////////
//////////    _sipReg = new SipRegister( _sipUser, SipUri( uristr ), _expires, qvalue );
//////////    connect( _sipReg, SIGNAL( statusUpdated() ), this, SLOT( registerStatusUpdated() ) );
//////////
//////////    _phoneObject->updateIdentity( _sipUser, _sipReg );
//////////    _sipClient->updateIdentity( _sipUser, _sipReg->getOutboundProxy() );
//////////
//////////    str = settings.value( pp + "/UserName" ).toString();
//////////    _sipUser->getMyUri()->setProxyUsername( str );
//////////    str = settings.value( pp + "/Password" ).toString();
//////////    _sipUser->getMyUri()->setPassword( str );
//////////    str = settings.value( pp + "/AutoRegister" ).toString();
//////////    if( str == "Yes" )
//////////    {
//////////      _autoRegister = true;
//////////      if( _useStun )
//////////      {
//////////        _sipReg->setAutoRegister( true );
//////////      }
//////////      else
//////////      {
//////////        _sipReg->requestRegister();
//////////      }
//////////    }
//////////    else
//////////    {
//////////      _autoRegister = false;
//////////      _sipReg->setAutoRegister( false );
//////////    }
//////////  }
//////////  else
//////////  {
//////////    editRegistration();
//////////  }
//////////}



RSipRegistrations::~RSipRegistrations( void )
{
	if(_pIdentityForm != NULL)
	{
		delete _pIdentityForm;
		_pIdentityForm = NULL;
	}
}

void RSipRegistrations::showIdentity( void )
{
  editRegistration();
}

void RSipRegistrations::save( void )
{
}

void RSipRegistrations::editRegistration( void )
{
  if( _sipRegister )
  {
    setRegisterState();
    _pIdentityForm->setFullname( _pSipUser->getUri().getFullname() );
    _pIdentityForm->setUsername( _pSipUser->getUri().getUsername() );
    _pIdentityForm->setHostname( _pSipUser->getUri().getHostname() );
    _pIdentityForm->setSipProxy( _sipRegister->getOutboundProxy() );
    _pIdentityForm->setSipProxyUsername( _pSipUser->getUri().getProxyUsername() );
    _pIdentityForm->setAutoRegister( _autoRegister );
    //edit->setQvalue( sipreg->getQvalue() );
  }
  else
  {
    _pIdentityForm->updateState( OFFLINE );
    _pIdentityForm->setFullname( "" );
    _pIdentityForm->setUsername( "" );
    _pIdentityForm->setHostname( "" );
    _pIdentityForm->setSipProxy( "" );
    _pIdentityForm->setSipProxyUsername( "" );
    //edit->setQvalue( "" );
    _pIdentityForm->setAutoRegister( true );
  }
  _pIdentityForm->show();
}

void RSipRegistrations::update( void )
{
  bool isDiff = false;
  QSettings settings;
  QString p = "/settings/Registration/";

  QString s = _pIdentityForm->getSipProxy();
  if( settings.value( p + "/SipUri", "" ).toString() != _pIdentityForm->getUri() ||
    settings.value( p + "/SipServer", "" ).toString() != _pIdentityForm->getSipProxy() ||
    settings.value( p + "/UserName", "" ).toString() != _pIdentityForm->getSipProxyUsername()// ||
    //settings.readEntry( p + "/qValue", "" ) != edit->getQvalue()
    )
  {
    isDiff = true;
  }
  settings.setValue( p + "/SipUri", _pIdentityForm->getUri() );
  settings.setValue( p + "/SipServer", _pIdentityForm->getSipProxy() );
  settings.setValue( p + "/UserName", _pIdentityForm->getSipProxyUsername() );
  if( _pIdentityForm->getAutoRegister() )
  {
    _autoRegister = true;
    settings.setValue( p + "/AutoRegister", "Yes");
  }
  else
  {
    _autoRegister = false;
    settings.setValue( p + "/AutoRegister", "No");
  }

  s = _pIdentityForm->getSipProxy();


  //settings.writeEntry( p + "/qValue", edit->getQvalue() );

  if( !_sipRegister )
  {
    QString uristr = _pIdentityForm->getUri();
    SipUri uri = SipUri( uristr );
    _pSipUser = _extSipClient->getUser( uri );
    if( _pSipUser == NULL )
    {
      _pSipUser = new SipUser( _extSipClient, uri );
    }
    uristr = _pIdentityForm->getSipProxy();
    QString qvalue = settings.value( p + "qValue", "" ).toString();
    _sipRegister = new SipRegister( _pSipUser, SipUri( uristr ), _expires, qvalue );
    connect( _sipRegister, SIGNAL( statusUpdated() ), this, SLOT( registerStatusUpdated() ) );
    _extPhoneObject->updateIdentity( _pSipUser, _sipRegister );
    _extSipClient->updateIdentity( _pSipUser, _sipRegister->getOutboundProxy() );
    QString str = _pIdentityForm->getSipProxyUsername();
    _pSipUser->getMyUri()->setProxyUsername( str );
    if( _pIdentityForm->getAutoRegister() )
    {
      _autoRegister = true;
      _sipRegister->requestRegister();
    }
    else
    {
      _autoRegister = false;
      _sipRegister->setAutoRegister( false );
    }
    if( _pIdentityForm->getAutoRegister() )
    {
      changeRegistration();
    }
  }
  else
  {
    if( isDiff )
    {
      QMessageBox::information( _parentWidget, tr("Identity"), tr("Restart RamblerPhone to apply identity changes.") );
    }
  }
}

void RSipRegistrations::changeRegistration( void )
{
  if( _sipRegister )
  {
    if( _sipRegister->getRegisterState() == SipRegister::Connected )
    {
      _sipRegister->requestClearRegistration();
    }
    else
    {
      _sipRegister->updateRegister();
      //view->setContactsOnline();
    }
  }
}


void RSipRegistrations::makeRegister( void )
{
  if( _sipRegister )
  {
    if( _sipRegister->getRegisterState() != SipRegister::Connected )
    {
      _sipRegister->updateRegister();
    }
  }
}

void RSipRegistrations::clearRegister( void )
{
  if( _sipRegister )
  {
    if( _sipRegister->getRegisterState() == SipRegister::Connected )
    {
      _sipRegister->requestClearRegistration();
    }
  }
}



void RSipRegistrations::unregAllRegistration( void )
{
  if( _sipRegister )
  {
    if( _sipRegister->getRegisterState() == SipRegister::Connected )
    {
      _sipRegister->requestClearRegistration();
    }
  }
}

void RSipRegistrations::setRegisterState( void )
{
  switch( _sipRegister->getRegisterState() )
  {
  case SipRegister::NotConnected:
    _pIdentityForm->updateState(  UNREG );
    break;
  case SipRegister::TryingServer:
  case SipRegister::TryingServerWithPassword:
    _pIdentityForm->updateState( PROC_TRY );
    break;
  case SipRegister::AuthenticationRequired:
  case SipRegister::AuthenticationRequiredWithNewPassword:
    _pIdentityForm->updateState( AUTHREQ );
    break;
  case SipRegister::Connected:
    _pIdentityForm->updateState( REG );
    break;
  case SipRegister::Disconnecting:
    _pIdentityForm->updateState( PROC_UNREG );
    break;
  case SipRegister::Reconnecting:
    _pIdentityForm->updateState( PROC_REG );
    break;
  }
}

void RSipRegistrations::registerStatusUpdated( void )
{
  setRegisterState();
  if( _sipRegister->getRegisterState() != SipRegister::AuthenticationRequired &&
    _sipRegister->getRegisterState() != SipRegister::AuthenticationRequiredWithNewPassword )
  {
    return;
  }
  QString authtype;
  switch( _sipRegister->getAuthenticationType() )
  {
  case SipRegister::DigestAuthenticationRequired:
    authtype = tr("Digest Authentication Request"); break;
  case SipRegister::BasicAuthenticationRequired:
    authtype = tr("Basic Authentication Request"); break;
  case SipRegister::ProxyDigestAuthenticationRequired:
    authtype = tr("Proxy Digest Authentication Request"); break;
  case SipRegister::ProxyBasicAuthenticationRequired:
    authtype = tr("Proxy Basic Authentication Request"); break;
  }

  QString server = _sipRegister->getServerUri().proxyUri();
  QString sipuri = _pSipUser->getUri().uri();

  KSipAuthenticationRequest authreq( server, sipuri, _userPrefix, authtype );

  QString username = _sipRegister->getRegisterCall()->getProxyUsername();
  authreq.setUsername( username );
  QString password = _sipRegister->getRegisterCall()->getPassword();
  if( password.isEmpty() || _sipRegister->getRegisterState() == SipRegister::AuthenticationRequiredWithNewPassword )
  {
    if( authreq.exec() )
    {
      if( authreq.getUsername().isEmpty() || authreq.getUsername().isEmpty() )
      {
        return;
      }
      _sipRegister->getRegisterCall()->setProxyUsername( authreq.getUsername() );
      _sipRegister->getRegisterCall()->setPassword( authreq.getPassword() );
      _sipRegister->requestRegister( authreq.getUsername(), authreq.getPassword() );
    }
  }
  else
  {
    _sipRegister->requestRegister( username, password );
  }
}
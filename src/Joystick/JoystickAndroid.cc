#include "JoystickAndroid.h"

#include "QGCApplication.h"

#include <QQmlEngine>

int JoystickAndroid::_androidBtnListCount;
int *JoystickAndroid::_androidBtnList;
int JoystickAndroid::ACTION_DOWN;
int JoystickAndroid::ACTION_UP;
QMutex JoystickAndroid::m_mutex;

static void clear_jni_exception()
{
    QAndroidJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }
}

JoystickAndroid::JoystickAndroid(const QString& name, int axisCount, int buttonCount, int hatCount, std::unordered_set<int> const&hatAxes, int id, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,axisCount,buttonCount,hatCount,multiVehicleManager)
    , hatCode{hatCount==0?nullptr: new int[2*hatCount]}
    , hatValue{hatCount==0?nullptr:new int[2*hatCount]}
    , deviceId(id)
{
    int i;
    
    QAndroidJniEnvironment env;
    QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);

    //set button mapping (number->code)
    jintArray b = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(b,0,_androidBtnListCount,_androidBtnList);

    QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", b);
    jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    //create a mapping table (btnCode) that maps button number with button code
    btnValue = new bool[_buttonCount];
    btnCode = new int[_buttonCount];
    int c = 0;
    for (i = 0; i < _androidBtnListCount; i++) {
        if (supportedButtons[i]) {
            btnValue[c] = false;
            btnCode[c] = _androidBtnList[i];
            c++;
        }
    }

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

    // set axis mapping (number->code)
    axisValue = new int[_axisCount];
    axisCode = new int[_axisCount];
    QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    int hatIndex=0;
    int axisIndex=0;
    for (i = 0; i < _axisCount + (_hatCount*2); i++) {
        QAndroidJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;",i);

        if(hatAxes.count(i))
        {
            hatCode[hatIndex] = range.callMethod<jint>("getAxis");

            // Don't allow two axis with the same code
            for (int j = 0; j < hatIndex; j++) {
                if (hatCode[hatIndex] == hatCode[j]) {
                    hatCode[hatIndex] = -1;
                    break;
                }
            }
            hatValue[hatIndex] = 0;
            ++hatIndex;
        }
        else
        {
            axisCode[axisIndex] = range.callMethod<jint>("getAxis");

            // Don't allow two axis with the same code
            for (int j = 0; j < axisIndex; j++) {
                if (axisCode[axisIndex] == axisCode[j]) {
                    axisCode[axisIndex] = -1;
                    break;
                }
            }
            axisValue[axisIndex] = 0;
            ++axisIndex;
        }
    }


    qCDebug(JoystickLog) << "axis:" <<_axisCount << " buttons:" <<_buttonCount<<" hats:"<<_hatCount;
    QtAndroidPrivate::registerGenericMotionEventListener(this);
    QtAndroidPrivate::registerKeyEventListener(this);

    if(name == "Kutta KTAC GC v2" || name == "Kutta KTAC GC v1" || name == "UXV Technologies SROC" || name == "Scuf Gaming SCUF Envision Controller" || name ==
        "Scuf Gaming SCUF Envision Pro Controller" || name == "EchoMAV EchoControl")
    {
         _setDefaultCalibration();
    }
}

JoystickAndroid::~JoystickAndroid() {
    delete[] btnCode;
    delete[] axisCode;
    delete[] hatCode;
    delete[] btnValue;
    delete[] axisValue;
    delete[] hatValue;

    QtAndroidPrivate::unregisterGenericMotionEventListener(this);
    QtAndroidPrivate::unregisterKeyEventListener(this);
}


QMap<QString, Joystick*> JoystickAndroid::discover(MultiVehicleManager* _multiVehicleManager) {
    static QMap<QString, Joystick*> ret;

    QMutexLocker lock(&m_mutex);

    QAndroidJniEnvironment env;
    QAndroidJniObject o = QAndroidJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = o.object<jintArray>();
    int sz = env->GetArrayLength(jarr);
    jint *buff = env->GetIntArrayElements(jarr, nullptr);

    int SOURCE_GAMEPAD = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    int SOURCE_JOYSTICK = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    QList<QString> names;

    for (int i = 0; i < sz; ++i) {
        QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        int sources = inputDevice.callMethod<jint>("getSources", "()I");
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) //check if the input device is interesting to us
                && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) continue;

        // get id and name
        QString id = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
        QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();
        QString toString = inputDevice.callObjectMethod("toString", "()Ljava/lang/String;").toString();
        if(name == "Kutta KTAC GC")
        {
            if(toString.contains("Descriptor: 1556518c36cc15a5d7cb0cc46112df69681182fb"))
            {
                name="Kutta KTAC GC v1";
            }
            else
            {
                name="Kutta KTAC GC v2";
            }
        }

        names.push_back(name);

        if (ret.contains(name)) {
            continue;
        }

        // get number of axis
        QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
        int axisCount = rangeListNative.callMethod<jint>("size");




        // get number of buttons
        jintArray a = env->NewIntArray(_androidBtnListCount);
        env->SetIntArrayRegion(a,0,_androidBtnListCount,_androidBtnList);
        QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", a);
        jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
        jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
        int buttonCount = 0;
        for (int j=0;j<_androidBtnListCount;j++)
            if (supportedButtons[j]) buttonCount++;
        env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

        qCDebug(JoystickLog) << "\t" << name << "id:" << buff[i] << "axes:" << axisCount << "buttons:" << buttonCount<<" toString:"<<toString;

        std::unordered_set<int> hatAxes;

        for(int j=0;j<axisCount;++j)
        {
            QAndroidJniObject motionRange = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;",j);
            jint axis = motionRange.callMethod<jint>("getAxis");
            jfloat flat = motionRange.callMethod<jfloat>("getFlat");
            jfloat fuzz = motionRange.callMethod<jfloat>("getFuzz");
            jfloat max = motionRange.callMethod<jfloat>("getMax");
            jfloat min = motionRange.callMethod<jfloat>("getMin");
            jfloat range = motionRange.callMethod<jfloat>("getRange");
            jfloat resolution = motionRange.callMethod<jfloat>("getResolution");
            jint source = motionRange.callMethod<jint>("getSource");
            qCDebug(JoystickLog)<<"\t\taxis:"<<axis<<" flat:"<<flat<<" fuzz:"<<fuzz<<" max:"<<max<<" min:"<<min<<" range: "<<range<<" resolution: "<<resolution<<" source: "<<source;
            if(fuzz==0 && flat==0)
            {
                hatAxes.insert(j);
            }

        }

        //currently only support 1 hat-switch
        int hatCount=1;
        axisCount-=2;


        //the Kutta v2 reports the hat switch as both buttons and an axis, so don't do this on that
        if(hatAxes.size()!=2 || name == "Kutta KTAC GC v2" )
        {
            hatAxes.clear();
            hatCount=0;
            axisCount+=2;
        }


        ret[name] = new JoystickAndroid(name, axisCount, buttonCount, hatCount, hatAxes, buff[i], _multiVehicleManager);
    }

    for (auto i = ret.begin(); i != ret.end();) {
        if (!names.contains(i.key())) {
            i = ret.erase(i);
        } else {
            i++;
        }
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);

    return ret;
}


bool JoystickAndroid::handleKeyEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    const int action = ev.callMethod<jint>("getAction", "()I");
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");

    for (int i = 0; i <_buttonCount; i++) {
        if (btnCode[i] == keyCode) {
            if (action == ACTION_DOWN) btnValue[i] = true;
            if (action == ACTION_UP)   btnValue[i] = false;
            return true;
        }
    }
    return false;
}

bool JoystickAndroid::handleGenericMotionEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    for (int i = 0; i <_axisCount; i++) {
        const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F",axisCode[i]);
        axisValue[i] = static_cast<int>((v*32767.f));
    }
    for(int i=0;i<_hatCount;++i)
    {
        for(int j=0;j<2;++j)
        {
            const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F",hatCode[i*2+j]);
            if(v>0)
            {
                hatValue[i*2+j]=1;
            }
            else if(v<0)
            {
                hatValue[i*2+j]=-1;
            }
            else
            {
                hatValue[i*2+j]=0;
            }
        }

    }
    return true;
}

bool JoystickAndroid::_open(void) {
    return true;
}

void JoystickAndroid::_close(void) {
}

bool JoystickAndroid::_update(void)
{
    return true;
}

bool JoystickAndroid::_getButton(int i) {
    return btnValue[ i ];
}

int JoystickAndroid::_getAxis(int i) {
    return axisValue[ i ];
}

bool JoystickAndroid::_getHat(int hat,int i) {
    if(hat<_hatCount)
    {
        switch(i)
        {
        case 0:
            //UP
            return hatValue[hat*2]<0;
        case 1:
            //DOWN
            return hatValue[hat*2]>0;
        case 2:
            //LEFT
            return hatValue[hat*2+1]<0;
        default:
            //RIGHT
            return hatValue[hat*2+1]>0;
        }
    }
    return false;
}

static JoystickManager *_manager = nullptr;

//helper method
bool JoystickAndroid::init(JoystickManager *manager) {
    _manager = manager;

    //this gets list of all possible buttons - this is needed to check how many buttons our gamepad supports
    //instead of the whole logic below we could have just a simple array of hardcoded int values as these 'should' not change

    //int JoystickAndroid::_androidBtnListCount;
    _androidBtnListCount = 31;
    static int ret[31]; //there are 31 buttons in total accordingy to the API
    int i;
    //int *JoystickAndroid::
    _androidBtnList = ret;

    clear_jni_exception();
    for (i = 1; i <= 16; i++) {
        QString name = "KEYCODE_BUTTON_"+QString::number(i);
        ret[i-1] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", name.toStdString().c_str());
    }
    i--;

    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_A");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_B");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_C");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L1");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L2");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R1");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R2");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_MODE");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_SELECT");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_START");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBL");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBR");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_X");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Y");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Z");

    ACTION_DOWN = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    ACTION_UP = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");

    return true;
}

static const char kJniClassName[] {"org/mavlink/qgroundcontrol/QGCActivity"};

static void jniUpdateAvailableJoysticks(JNIEnv *envA, jobject thizA)
{
    Q_UNUSED(envA);
    Q_UNUSED(thizA);

    if (_manager != nullptr) {
        qCDebug(JoystickLog) << "jniUpdateAvailableJoysticks triggered";
        emit _manager->updateAvailableJoysticksSignal();
    }
}

void JoystickAndroid::setNativeMethods()
{
    qCDebug(JoystickLog) << "Registering Native Functions";

    //  REGISTER THE C++ FUNCTION WITH JNI
    JNINativeMethod javaMethods[] {
        {"nativeUpdateAvailableJoysticks", "()V", reinterpret_cast<void *>(jniUpdateAvailableJoysticks)}
    };

    clear_jni_exception();
    QAndroidJniEnvironment jniEnv;
    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if(!objectClass) {
        clear_jni_exception();
        qWarning() << "Couldn't find class:" << kJniClassName;
        return;
    }

    jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));

    if (val < 0) {
        qWarning() << "Error registering methods: " << val;
    } else {
        qCDebug(JoystickLog) << "Native Functions Registered";
    }
    clear_jni_exception();
}

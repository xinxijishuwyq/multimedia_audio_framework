import router from '@system.router';
import nativeapis from 'libnative.so';

export default {
    data: {
        title: "Hello world",
        introduction: "Adjust the slider and click the Button, the program will call the native APIs (OpenSL ES and "
        + "HiLog).",
        currentValue: 0,
    },
    onInit() {
    },
    backToIndex() {
        router.replace({
            uri: 'pages/index/index',
        })
    },
    onSliderChange(e) {
        this.currentValue = e.value;
    },
    onButtonClick() {
        console.log("[HiLog] " + this.currentValue);
        this.title = nativeapis.create(this.currentValue);
    }
}

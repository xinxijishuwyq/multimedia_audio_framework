import router from '@system.router';

export default {
    data: {
        title: ""
    },

    toPlayer() {
        router.push({
            uri: 'pages/player/player',
        })
    },

    toNative() {
        router.push({
            uri: 'pages/native/native',
        })
    },
	
	toStreamer() {
        router.push({
            uri: 'pages/streamer/streamer',
        })
    }
}

import threading

from flask import Flask, Response

app = Flask(__name__)
output_frame = None
lock = threading.Lock()


def generate():
    global output_frame, lock
    while True:
        with lock:
            if output_frame is None:
                continue
            # Encapsula os bytes do JPEG no protocolo MJPEG
            yield (
                b"--frame\r\nContent-Type: image/jpeg\r\n\r\n" + output_frame + b"\r\n"
            )


@app.route("/video_feed")
def video_feed():
    return Response(generate(), mimetype="multipart/x-mixed-replace; boundary=frame")


def start_server(host="0.0.0.0", port=5000):
    app.run(host=host, port=port, debug=False, threaded=True, use_reloader=False)

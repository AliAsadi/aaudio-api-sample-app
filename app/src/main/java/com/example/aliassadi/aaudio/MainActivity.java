package com.example.aliassadi.aaudio;

import android.content.res.AssetManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private boolean isPlaying;
    private Button button;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        AAudioApi.init();

        short[] buffer = readFile();

        AAudioApi.writeBuffer(buffer);

        button = findViewById(R.id.button);
        button.setOnClickListener(this);
    }

    private short[] readFile() {
        //fill buffer
        AssetManager am = getBaseContext().getAssets();
        try {
            InputStream is = am.open("myfile");
            byte[] arr = readBytes(is);
            short[] shorts = new short[arr.length / 2];
            ByteBuffer.wrap(arr).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
            return shorts;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }
    public static byte[] readBytes(InputStream inputStream) throws IOException {
        byte[] b = new byte[1024];
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        int c;
        while ((c = inputStream.read(b)) != -1) {
            os.write(b, 0, c);
        }
        return os.toByteArray();
    }

    @Override
    public void onClick(View view) {
        if (isPlaying) {
            AAudioApi.stop();

            button.setText("start");
            isPlaying = false;
        } else {
            AAudioApi.start();

            button.setText("stop");
            isPlaying = true;
        }
    }
}

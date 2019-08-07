//Include C++ libraries
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <utility>
#include <ctime>
#include <dirent.h>
#include <stdio.h>

//Include gstreamer libraries
#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <thread>
using namespace std;
GError* errRead = NULL;
GMainLoop *loopRead  = NULL;
GstPipeline *pipelineRead = NULL;
GstElement* mPipelineRead = NULL;
GError* errProcess = NULL;
GMainLoop *loopProcess  = NULL;
GstPipeline *pipelineProcess = NULL;
GstElement* mPipelineProcess = NULL;
bool playAudio = false;
static GstFlowReturn on_audio_buffer(GstAppSink *vsink, gpointer user_data){

    GstSample *sample = gst_app_sink_pull_sample((GstAppSink*) vsink);
    GstElement *m_src = NULL;
    if(mPipelineProcess == NULL){
#ifdef DEBUG
        g_print("Fail to get element \n");
#endif
        return GST_FLOW_NOT_LINKED;
    }
    m_src = gst_bin_get_by_name((GstBin*)mPipelineProcess, "audio-play");

    if(m_src == NULL)
    {
#ifdef DEBUG
        g_print("Fail to get element \n");
#endif
        return GST_FLOW_NOT_LINKED;
    }
    GstAppSrc *m_appsrc = (GstAppSrc *)m_src;
    if(!playAudio){
        return GST_FLOW_OK;
    }else{
        gst_app_src_push_sample(m_appsrc,sample);
    }

    gst_sample_unref (sample);
//    printf("Received new audio sample\r\n");
    return GST_FLOW_OK;
}

// onEOS
void on_audio_EOS(_GstAppSink* sink, void* user_data)
{
    printf( "gstreamer decoder onEOS\r\n");
}

// onPreroll
GstFlowReturn on_audio_preroll(_GstAppSink* sink, void* user_data)
{
    printf( "gstreamer decoder onPreroll\n");
    return GST_FLOW_OK;
}
void audio_controller(){
    while(1){
        playAudio =! playAudio;
        sleep(5);
    }
}
bool reading_thread(){
    pipelineRead = GST_PIPELINE(gst_pipeline_new(nullptr));
    if(pipelineRead == NULL){
        g_print("gst_pipeline_new failed\r\n");
    }else
        g_print("gst_pipeline_new done\r\n");
    loopRead = g_main_loop_new(NULL, FALSE);
    std::ostringstream ss;
    ss << "filesrc location=Duyen-Minh-Lo-Huong-Tram.mp3 ! mpegaudioparse ! mpg123audiodec ! appsink name=audio-capture";
    mPipelineRead = gst_parse_launch(ss.str().c_str(), &errRead);
    if( errRead != NULL )
    {
        g_print("gstreamer decoder failed to create pipeline\n");
        g_error_free(errRead);
        return FALSE;
    }else{
        g_print("gstreamer decoder create pipeline success\n");
    }
    pipelineRead = GST_PIPELINE(mPipelineRead);
    GstAppSinkCallbacks cb;
    memset(&cb, 0, sizeof(GstAppSinkCallbacks));

    cb.eos         = on_audio_EOS;
    cb.new_preroll = on_audio_preroll;
    cb.new_sample  = on_audio_buffer;
    GstElement *m_sink = gst_bin_get_by_name((GstBin*)mPipelineRead, "audio-capture");
    GstAppSink *m_appsink = (GstAppSink *)m_sink;
    if(!m_sink || !m_appsink)
    {
#ifdef DEBUG
        g_print("Fail to get element \n");
#endif
        return FALSE;
    }
    gst_app_sink_set_callbacks(m_appsink, &cb, loopRead, NULL);
    const GstStateChangeReturn result = gst_element_set_state(mPipelineRead, GST_STATE_PLAYING);
    if(result != GST_STATE_CHANGE_SUCCESS)
    {
        g_print("gstreamer failed to playing\n");
    }
    g_main_loop_run(loopRead);
    gst_element_set_state(GST_ELEMENT(mPipelineRead), GST_STATE_NULL);
    g_object_unref(mPipelineRead);
    printf("audio read done\n");
}
bool processing_thread(){
    pipelineProcess = GST_PIPELINE(gst_pipeline_new(nullptr));
    if(pipelineProcess == NULL){
        g_print("gst_pipeline_new failed\r\n");
    }else
        g_print("gst_pipeline_new done\r\n");
    loopProcess = g_main_loop_new(NULL, FALSE);
    std::ostringstream ss;
    ss << "appsrc name=audio-play ! alsasink sync=false";
    mPipelineProcess = gst_parse_launch(ss.str().c_str(), &errProcess);
    if( errProcess != NULL )
    {
        g_print("gstreamer decoder failed to create pipeline\n");
        g_error_free(errProcess);
        return FALSE;
    }else{
        g_print("gstreamer decoder create pipeline success\n");
    }
    pipelineProcess = GST_PIPELINE(mPipelineProcess);
    const GstStateChangeReturn result = gst_element_set_state(mPipelineProcess, GST_STATE_PLAYING);
    if(result != GST_STATE_CHANGE_SUCCESS)
    {
        g_print("gstreamer failed to playing\n");
    }
    g_main_loop_run(loopProcess);
    gst_element_set_state(GST_ELEMENT(mPipelineProcess), GST_STATE_NULL);
    g_object_unref(mPipelineProcess);
    printf("gstreamer setup done\n");
}
int main()
{
    cout << "Hello World!" << endl;
    gst_init(0,NULL);
    std::thread t1(reading_thread);
    std::thread t2(processing_thread);
    std::thread t3(audio_controller);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}

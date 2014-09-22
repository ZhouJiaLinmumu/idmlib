#ifndef _PARAGRAPH_VEC_H_
#define _PARAGRAPH_VEC_H_

#include <string>
#include <vector>


struct ModelArgument
{
    long long layer1_size;
    std::string train_file;
    std::string save_vocab_file;
    std::string read_vocab_file;
    int cbow;
    float alpha; //default 0.025
    std::string output_file;
    int window;
    float sample;
    int hs;
    int negative;
    int num_threads;
    int min_count;

    ModelArgument():
        layer1_size(200),
        cbow(1),
        alpha(0.025),
        window(5),
        sample(0),
        hs(1),
        negative(0),
        num_threads(1),
        min_count(5)
    {}

};




class ParagraphVector
{

public:
    ParagraphVector(const ModelArgument& ma);
    ~ParagraphVector();

public:

    bool Init(bool bPredict);

    void TrainModel();

    std::vector<float> PredictPara(const std::string& para);

private:
    ModelArgument ma_;

};


#endif

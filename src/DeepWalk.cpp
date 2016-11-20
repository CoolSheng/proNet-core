#include "DeepWalk.h"

DeepWalk::DeepWalk () {
}

DeepWalk::~DeepWalk () {
}

void DeepWalk::LoadEdgeList(string filename, bool undirect) {
    rgraph.LoadEdgeList(filename, undirect);
}

void DeepWalk::SaveWeights(string model_name){
    
    cout << "Save Model:" << endl;
    ofstream model(model_name);
    if (model)
    {
        for (auto k: rgraph.keys)
        {
            model << k;
            for (int d=0; d<dim; ++d)
                model << " " << w_vertex[rgraph.kmap[k]][d];
            model << endl;
        }
        cout << "\tSave to <" << model_name << ">" << endl;
    }
    else
    {
        cout << "\tfail to open file" << endl;
    }
}

void DeepWalk::Init(int dim) {
    
    this->dim = dim;
    cout << "Model Setting:" << endl;
    cout << "\tdimension:\t\t" << dim << endl;
    
    w_vertex.resize(rgraph.MAX_vid);
    w_context.resize(rgraph.MAX_vid);

    for (long vid=0; vid<rgraph.MAX_vid; ++vid)
    {
        w_vertex[vid].resize(dim);
        for (int d=0; d<dim;++d)
            w_vertex[vid][d] = (rand()/(double)RAND_MAX - 0.5) / dim;
    }

    for (long vid=0; vid<rgraph.MAX_vid; ++vid)
    {
        w_context[vid].resize(dim);
        for (int d=0; d<dim;++d)
            w_context[vid][d] = (rand()/(double)RAND_MAX - 0.5) / dim;
    }
}

void DeepWalk::Train(int walk_times, int walk_steps, int window_size, int negative_samples, double alpha, int workers){
    
    omp_set_num_threads(workers);

    cout << "Model:" << endl;
    cout << "\tDeepWalk" << endl;

    cout << "Learning Parameters:" << endl;
    cout << "\twalk_times:\t\t" << walk_times << endl;
    cout << "\twalk_steps:\t\t" << walk_steps << endl;
    cout << "\twindow_size:\t\t" << window_size << endl;
    cout << "\tnegative_samples:\t" << negative_samples << endl;
    cout << "\talpha:\t\t\t" << alpha << endl;
    cout << "\tworkers:\t\t" << workers << endl;

    cout << "Start Training:" << endl;


    long total = walk_times*rgraph.MAX_vid;
    double alpha_min = alpha*0.0001;
    double _alpha = alpha;
    long count = 0;

    for (int t=0; t<walk_times; ++t)
    {
        // shuffle the order for random keys access        
        std::vector<long> random_keys(rgraph.MAX_vid);
        for (long vid = 0; vid < rgraph.MAX_vid; vid++) {
            random_keys[vid] = vid;
        }
        for (long vid = 0; vid < rgraph.MAX_vid; vid++) {
            int rdx = vid + rand() % (rgraph.MAX_vid - vid); // careful here!
            swap(random_keys[vid], random_keys[rdx]);
        }

        #pragma omp parallel for
        for (long vid=0; vid<rgraph.MAX_vid; ++vid)
        {
            if (_alpha < alpha_min) _alpha = alpha_min;

            vector<long> walks = rgraph.RandomWalk(random_keys[vid], walk_steps);
            vector<vector<long>> train_data = rgraph.SkipGrams(walks, window_size, 0);
            rgraph.UpdatePairs(w_vertex, w_context, train_data[0], train_data[1], dim, negative_samples, _alpha);
            
            count++;
            if (count % MONITOR == 0)
            {
                _alpha = alpha* ( 1.0 - (double)(count)/total );
                printf("\tAlpha: %.6f\tProgress: %.3f %%%c", _alpha, (double)(count)/total * 100, 13);
                fflush(stdout);
            }

        }

    }
    printf("\tAlpha: %.6f\tProgress: 100.00 %%\n", _alpha);

}


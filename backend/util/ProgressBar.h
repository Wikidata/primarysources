//
// Created by wastl on 24.04.16.
//

#ifndef PRIMARYSOURCES_PROGRESSBAR_H
#define PRIMARYSOURCES_PROGRESSBAR_H
namespace wikidata {
namespace primarysources {

// Show an ASCII progress bar.
class ProgressBar {
 public:
    ProgressBar(int width, long max) : width(width), max(max) {}
    ~ProgressBar();

    void Update(long value);

 private:
    int width;
    long max;
};

}  // namespace primarysources
}  // namespaec wikidata
#endif //PRIMARYSOURCES_PROGRESSBAR_H

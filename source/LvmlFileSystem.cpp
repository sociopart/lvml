#include "LvmlFileSystem.hpp"
#include "LvmlMemoryResources.hpp"

lv_fs_drv_t     CLvmlFileSystem::s_driver;
CLvmlResources* CLvmlFileSystem::s_resources  = nullptr;
char            CLvmlFileSystem::s_letter     = 'L';
bool            CLvmlFileSystem::s_registered = false;

void CLvmlFileSystem::Register(char letter, CLvmlResources* resources)
{
    s_resources = resources;
    if (s_registered && letter == s_letter) return;
    s_letter = letter;

    lv_fs_drv_init(&s_driver);
    s_driver.letter   = letter;
    s_driver.ready_cb = Ready;
    s_driver.open_cb  = Open;
    s_driver.close_cb = Close;
    s_driver.read_cb  = Read;
    s_driver.seek_cb  = Seek;
    s_driver.tell_cb  = Tell;
    lv_fs_drv_register(&s_driver);
    s_registered = true;
}

std::string CLvmlFileSystem::MakePath(std::string_view resourcePath)
{
    std::string out;
    out += s_letter;
    out += ':';
    out += CLvmlResources::NormalizePath(resourcePath);
    return out;
}

bool CLvmlFileSystem::Ready(lv_fs_drv_t*)
{
    return s_resources != nullptr;
}

void* CLvmlFileSystem::Open(lv_fs_drv_t*, const char* path, lv_fs_mode_t mode)
{
    if (!s_resources || (mode & LV_FS_MODE_WR)) return nullptr;
    auto* file = new SFile();
    if (auto* memory = dynamic_cast<CLvmlMemoryResources*>(s_resources)) {
        file->data = memory->Find(path);
    } else if (s_resources->Read(path, file->owned)) {
        file->data = &file->owned;
    }
    if (!file->data) { delete file; return nullptr; }
    return file;
}

lv_fs_res_t CLvmlFileSystem::Close(lv_fs_drv_t*, void* file)
{
    delete static_cast<SFile*>(file);
    return LV_FS_RES_OK;
}

lv_fs_res_t CLvmlFileSystem::Read(lv_fs_drv_t*, void* file, void* buf, uint32_t btr, uint32_t* br)
{
    auto* f = static_cast<SFile*>(file);
    const uint32_t size = uint32_t(f->data->size());
    const uint32_t n    = f->pos >= size ? 0 : (btr < size - f->pos ? btr : size - f->pos);
    if (n) lv_memcpy(buf, f->data->data() + f->pos, n);
    f->pos += n;
    if (br) *br = n;
    return LV_FS_RES_OK;
}

lv_fs_res_t CLvmlFileSystem::Seek(lv_fs_drv_t*, void* file, uint32_t pos, lv_fs_whence_t whence)
{
    auto* f = static_cast<SFile*>(file);
    const int64_t size = int64_t(f->data->size());
    int64_t target = pos;
    if (whence == LV_FS_SEEK_CUR) target = int64_t(f->pos) + int32_t(pos);
    else if (whence == LV_FS_SEEK_END) target = size + int32_t(pos);
    if (target < 0) target = 0;
    if (target > size) target = size;
    f->pos = uint32_t(target);
    return LV_FS_RES_OK;
}

lv_fs_res_t CLvmlFileSystem::Tell(lv_fs_drv_t*, void* file, uint32_t* pos)
{
    *pos = static_cast<SFile*>(file)->pos;
    return LV_FS_RES_OK;
}
